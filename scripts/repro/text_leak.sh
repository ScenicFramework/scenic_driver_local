#!/usr/bin/env bash
# Build the cairo-gtk driver and run the draw_text leak reproduction.
# See scripts/repro/README.md. Extra env vars (FRAMES, TEXTS, LEAK_FRAMES,
# SKIP_LEAKS) are passed through to text_leak.exs.
set -euo pipefail

cd "$(dirname "$0")/../.."

export SCENIC_LOCAL_TARGET=cairo-gtk
export MIX_ENV="${MIX_ENV:-dev}"

# The Makefile only rebuilds when a C source changes, not when the target
# changes, so remove any binary built for another target.
rm -f "_build/$MIX_ENV/lib/scenic_driver_local/priv/scenic_driver_local"

mix deps.get >/dev/null
mix compile

echo "driver commit: $(git rev-parse --short HEAD)$(git diff --quiet HEAD -- c_src || echo ' (c_src modified)')"
mix run --no-start scripts/repro/text_leak.exs
