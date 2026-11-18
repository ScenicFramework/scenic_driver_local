#!/usr/bin/env bash
# Build the cairo-gtk driver and run one of the scripts in scripts/repro
# against it:
#
#   scripts/repro/run.sh SCRIPT.exs
#
# Environment variables are passed through to the script. See README.md.
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

export SCENIC_LOCAL_TARGET=cairo-gtk
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
if [ "$(uname -s)" = "Linux" ] && [ -z "${REPRO_SHOW_WINDOW:-}" ]; then
  if have xvfb-run; then
    echo "running on a virtual display (xvfb-run); set REPRO_SHOW_WINDOW=1 to see the windows"
    run=(env -u WAYLAND_DISPLAY GDK_BACKEND=x11 xvfb-run -a)
  elif [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
    echo "warning: no display and no xvfb-run; the port will probably fail to start" >&2
  else
    echo "warning: xvfb-run not found; the port's windows will open on your desktop" >&2
  fi
fi

echo "driver commit: $(git rev-parse --short HEAD)$(git diff --quiet HEAD -- c_src || echo ' (c_src modified)')"
${run[@]+"${run[@]}"} mix run --no-start "scripts/repro/$script"
