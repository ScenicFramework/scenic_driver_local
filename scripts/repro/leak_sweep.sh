#!/usr/bin/env bash
# Build the cairo-gtk driver and sweep its script ops and port commands for
# leaks. See scripts/repro/README.md. Env vars (CASES, ITERATIONS, WARMUP,
# LEAK_TOOL, LIST) are passed through to leak_sweep.exs.
exec "$(dirname "$0")/run.sh" leak_sweep.exs
