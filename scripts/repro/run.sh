#!/usr/bin/env bash
# Build the driver and run one of the scripts in scripts/repro against it:
#
#   scripts/repro/run.sh SCRIPT.exs
#
# REPRO_TARGET picks the backend: cairo-gtk (the default) or cairo-fb, which
# also needs REPRO_FBDEV set to a framebuffer device. Other environment
# variables are passed through to the script. See README.md.
#
# macOS: runs natively; the scripts attribute leaks with `leaks`.
# Linux: the scripts attribute leaks with valgrind or heaptrack. If one of
#        those (or the build dependencies) is missing and nix is installed,
#        this re-runs itself inside the flake's dev shell, which provides
#        them. Set REPRO_NO_NIX=1 to stay in the current environment.
set -euo pipefail

script="$1"
cd "$(dirname "$0")/../.."

have() { command -v "$1" >/dev/null 2>&1; }

# TEXT_LEAK_NO_NIX is the older name for REPRO_NO_NIX.
no_nix="${REPRO_NO_NIX:-${TEXT_LEAK_NO_NIX:-}}"

if [ "$(uname -s)" = "Linux" ]; then
  missing=()
  for tool in mix pkg-config valgrind heaptrack; do
    have "$tool" || missing+=("$tool")
  done

  if [ "${#missing[@]}" -gt 0 ] && [ -z "$no_nix" ] && have nix && [ -f flake.nix ]; then
    echo "not on PATH: ${missing[*]}; re-running inside the flake dev shell (nix develop)"
    # REPRO_NO_NIX stops a second re-exec if the dev shell lacks a tool too.
    REPRO_NO_NIX=1 exec nix develop --command scripts/repro/run.sh "$script"
  fi
fi

target="${REPRO_TARGET:-cairo-gtk}"

case "$target" in
  cairo-gtk) ;;
  cairo-fb)
    # The port draws straight into this device, so it must be one that isn't
    # anybody's screen: the kernel's virtual framebuffer (modprobe vfb).
    fbdev="${REPRO_FBDEV:-}"
    if [ -z "$fbdev" ]; then
      echo "REPRO_TARGET=cairo-fb needs REPRO_FBDEV, e.g. REPRO_FBDEV=/dev/fb1" >&2
      echo "create a virtual framebuffer with: sudo modprobe vfb vfb_enable=1" >&2
      exit 1
    fi
    fbname="$(cat "/sys/class/graphics/$(basename "$fbdev")/name" 2>/dev/null || true)"
    if [ "$fbname" != "Virtual FB" ] && [ -z "${REPRO_ALLOW_REAL_FB:-}" ]; then
      echo "$fbdev is '${fbname:-unknown}', not a virtual framebuffer; the port would draw over a real screen." >&2
      echo "set REPRO_ALLOW_REAL_FB=1 if that is what you want" >&2
      exit 1
    fi
    # The port retries a failed open for 60 seconds, so check first.
    if [ ! -r "$fbdev" ] || [ ! -w "$fbdev" ]; then
      echo "no read/write access to $fbdev ($(ls -l "$fbdev" 2>/dev/null))" >&2
      exit 1
    fi
    export REPRO_FBDEV="$fbdev"
    # The Makefile builds this target without debug info, and the leak tools
    # need it to tell driver frames from library frames.
    export CFLAGS="${CFLAGS:--O2 -g}"
    ;;
  *)
    echo "REPRO_TARGET=$target is not supported (cairo-gtk or cairo-fb)" >&2
    exit 1
    ;;
esac

export SCENIC_LOCAL_TARGET="$target"
export MIX_ENV="${MIX_ENV:-dev}"

# The Makefile only rebuilds when a C source changes, not when the target
# changes, so remove any binary built for another target.
rm -f "_build/$MIX_ENV/lib/scenic_driver_local/priv/scenic_driver_local"

mix deps.get >/dev/null
mix compile

# The port opens a GTK window, and the scripts start it many times over. On
# Linux run it on a virtual display, so the windows neither appear on the
# desktop nor take input focus. REPRO_SHOW_WINDOW=1 uses the real display.
# (the ${run[@]+...} form below keeps bash 3.2 on macOS happy under set -u)
run=()
if [ "$target" = "cairo-gtk" ] && [ "$(uname -s)" = "Linux" ] && [ -z "${REPRO_SHOW_WINDOW:-}" ]; then
  if have xvfb-run; then
    echo "running on a virtual display (xvfb-run); set REPRO_SHOW_WINDOW=1 to see the windows"
    run=(env -u WAYLAND_DISPLAY GDK_BACKEND=x11 xvfb-run -a)
  elif [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
    echo "warning: no display and no xvfb-run; the port will probably fail to start" >&2
  else
    echo "warning: xvfb-run not found; the port's windows will open on your desktop" >&2
  fi
fi

echo "driver commit: $(git rev-parse --short HEAD)$(git diff --quiet HEAD -- c_src || echo ' (c_src modified)'), target: $target"
${run[@]+"${run[@]}"} mix run --no-start "scripts/repro/$script"
