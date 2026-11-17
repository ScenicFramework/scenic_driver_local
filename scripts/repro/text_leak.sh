#!/usr/bin/env bash
# Build the cairo-gtk driver and run the draw_text leak reproduction.
# See scripts/repro/README.md. Extra env vars (FRAMES, TEXTS, LEAK_FRAMES,
# LEAK_TOOL, SKIP_LEAKS, PORT_WRAPPER) are passed through to text_leak.exs.
#
# macOS: runs natively and attributes leaks with `leaks`.
# Linux: attributes leaks with valgrind or heaptrack. If one of those (or the
#        build dependencies) is missing and nix is installed, the script
#        re-runs itself inside the flake's dev shell, which provides them.
#        Set TEXT_LEAK_NO_NIX=1 to stay in the current environment.
set -euo pipefail

cd "$(dirname "$0")/../.."

have() { command -v "$1" >/dev/null 2>&1; }

if [ "$(uname -s)" = "Linux" ]; then
  missing=()
  for tool in mix pkg-config valgrind heaptrack; do
    have "$tool" || missing+=("$tool")
  done

  if [ "${#missing[@]}" -gt 0 ] && [ -z "${TEXT_LEAK_NO_NIX:-}" ] && have nix && [ -f flake.nix ]; then
    echo "not on PATH: ${missing[*]}; re-running inside the flake dev shell (nix develop)"
    # TEXT_LEAK_NO_NIX stops a second re-exec if the dev shell lacks a tool too.
    TEXT_LEAK_NO_NIX=1 exec nix develop --command "scripts/repro/text_leak.sh" "$@"
  fi
fi

export SCENIC_LOCAL_TARGET=cairo-gtk
export MIX_ENV="${MIX_ENV:-dev}"

# The Makefile only rebuilds when a C source changes, not when the target
# changes, so remove any binary built for another target.
rm -f "_build/$MIX_ENV/lib/scenic_driver_local/priv/scenic_driver_local"

mix deps.get >/dev/null
mix compile

# The port opens a GTK window. With no display (ssh, CI), use a virtual one.
# (the ${run[@]+...} form below keeps bash 3.2 on macOS happy under set -u)
run=()
if [ "$(uname -s)" = "Linux" ] && [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
  if have xvfb-run; then
    echo "no DISPLAY or WAYLAND_DISPLAY; running under xvfb-run"
    run=(xvfb-run -a)
  else
    echo "warning: no DISPLAY or WAYLAND_DISPLAY and no xvfb-run; the port will probably fail to start" >&2
  fi
fi

echo "driver commit: $(git rev-parse --short HEAD)$(git diff --quiet HEAD -- c_src || echo ' (c_src modified)')"
${run[@]+"${run[@]}"} mix run --no-start scripts/repro/text_leak.exs
