#!/usr/bin/env bash
# Run a port program under heaptrack without touching its stdout.
#
#   heaptrack_port.sh OUTPUT COMMAND [ARG]...
#
# The stock `heaptrack` launcher prints status lines to stdout before it starts
# the program. A port's stdout is its {:packet, 4} channel back to the BEAM, so
# those lines corrupt the protocol. This does what the launcher does (preload
# libheaptrack, pipe its trace through heaptrack_interpret into a compressed
# file) and keeps stdout for the program alone.
#
# OUTPUT should end in .zst or .gz; read it with `heaptrack_print OUTPUT`.
set -euo pipefail

output="$1"
shift

launcher="$(command -v heaptrack)" || {
  echo "heaptrack_port.sh: heaptrack not found on PATH" >&2
  exit 127
}
prefix="$(dirname "$(dirname "$(readlink -f "$launcher")")")"

# The library and interpreter sit under lib*/ and lib*/ or libexec/ depending
# on the distro, so search the install prefix for them.
preload="$(find "$prefix" -name libheaptrack_preload.so -print -quit 2>/dev/null)"
interpret="$(find "$prefix" -name heaptrack_interpret -print -quit 2>/dev/null)"
if [ -z "$preload" ] || [ -z "$interpret" ]; then
  echo "heaptrack_port.sh: no libheaptrack_preload.so / heaptrack_interpret under $prefix" >&2
  exit 127
fi

case "$output" in
  *.zst) compress=(zstd -c) ;;
  *) compress=(gzip -c) ;;
esac

fifo="$(mktemp -u "${TMPDIR:-/tmp}/heaptrack_port_fifo.XXXXXX")"
mkfifo "$fifo"
trap 'rm -f "$fifo"' EXIT

"$interpret" <"$fifo" 2>/dev/null | "${compress[@]}" >"$output" &

status=0
LD_PRELOAD="$preload${LD_PRELOAD:+:$LD_PRELOAD}" DUMP_HEAPTRACK_OUTPUT="$fifo" "$@" || status=$?

# The interpreter sees EOF once the program has exited; wait for it to finish
# writing the trace.
wait
exit "$status"
