#!/usr/bin/env bash
# Build the cairo-gtk driver and run the draw_text leak reproduction.
# See scripts/repro/README.md. Env vars (FRAMES, TEXTS, LEAK_FRAMES, LEAK_TOOL,
# SKIP_LEAKS, PORT_WRAPPER) are passed through to text_leak.exs.
exec "$(dirname "$0")/run.sh" text_leak.exs
