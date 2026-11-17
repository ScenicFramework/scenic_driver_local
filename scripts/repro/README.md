# draw_text leak reproduction (cairo)

`text_leak.sh` checks the cairo backend's `script_ops_draw_text` for a
per-frame leak. `cairo_scaled_font_text_to_glyphs` returns a glyph array and a
cluster array that the caller owns; if `draw_text` doesn't free them, every
call leaks both.

It builds the real driver with `SCENIC_LOCAL_TARGET=cairo-gtk`, starts the
port binary directly, and talks to it over the normal port protocol (no
ViewPort). The port gets a font, a clear colour and a `_root_` script that sets
a fill colour and font and then calls `draw_text` `TEXTS` times. Each frame is
one `render` command; the script waits for the port's `ready` reply before
sending the next, so every frame is actually drawn. A GTK window opens while
it runs.

It runs two phases, each in a fresh port process:

1. **RSS.** 200 warm-up frames, then `FRAMES` frames, printing the port's RSS
   (from `ps`) every 1,000 frames and the average growth per 1,000 frames.
2. **Leak attribution (macOS only).** Starts the port with
   `MallocStackLogging=1`, renders 200 + `LEAK_FRAMES` frames, runs
   `leaks <pid>` and summarises the leaked blocks grouped by allocation site
   and innermost driver frame, labelled where the stack is recognised. The
   full `leaks` output is saved to a temp file whose path is printed.

The script's `fill_color` op also leaks a `cairo_pattern_t` per frame
(a separate bug: every fill/stroke colour or gradient op leaks a pattern). It
is kept in on purpose so the report shows the two leaks apart: text leaks are
allocated in `cairo_scaled_font_text_to_glyphs` under `script_ops_draw_text`,
pattern leaks in `_cairo_pattern_create_*` under `script_ops_fill_color`. RSS
alone can't tell them apart; the `leaks` grouping can.

## Prerequisites

- Elixir and Erlang matching `mix.exs` (tested with Elixir 1.16.3 / OTP 26).
- macOS: `brew install gtk+3 cairo freetype pkg-config`. `leaks` ships with the
  Xcode command line tools.
- Linux: `apt install libgtk-3-dev libcairo2-dev libfreetype-dev pkg-config`
  (or your distro's equivalents) and a display; `xvfb-run` works if there is
  no desktop session.

## Running

```sh
scripts/repro/text_leak.sh
```

Settings are environment variables:

| Variable       | Default | Meaning                                         |
| -------------- | ------- | ----------------------------------------------- |
| `FRAMES`       | 5000    | frames in the RSS phase                         |
| `TEXTS`        | 20      | `draw_text` calls per frame                     |
| `LEAK_FRAMES`  | 100     | frames after warm-up in the `leaks` phase       |
| `SKIP_LEAKS`   | unset   | `1` skips the `leaks` phase                     |
| `PORT_WRAPPER` | unset   | command the RSS-phase port runs under           |

To compare before and after a C change, run it on each commit; the script
removes the built port binary first so the C code is always rebuilt.

## Reading the output

With the defaults, a leaking `draw_text` shows in `leaks` as about 40 blocks
per frame (one glyph array and one cluster array for each of the 20 calls)
from `cairo_scaled_font_text_to_glyphs -> script_ops_draw_text`, and RSS grows
by about 12.8 MB per 1,000 frames. Without the leak those rows are absent.
The `script_ops_fill_color` pattern row is the separate pattern leak, and a few
patterns allocated by GTK's own window drawing (innermost driver frame `main`)
also show up; those are not driver code.

## Linux: valgrind or heaptrack

`leaks` is macOS-only. On Linux, use `PORT_WRAPPER` to run the RSS-phase port
under a heap tool; the port exits cleanly when the phase ends, so the tool
writes its report then. Keep `FRAMES` small, since valgrind is slow.

```sh
# valgrind: "definitely lost" records with full allocation stacks
SKIP_LEAKS=1 FRAMES=1000 \
  PORT_WRAPPER="valgrind --leak-check=full --num-callers=30 --log-file=vg.%p.txt" \
  xvfb-run -a scripts/repro/text_leak.sh
grep -A25 "definitely lost" vg.*.txt | grep -B2 -A20 cairo_scaled_font_text_to_glyphs

# heaptrack: leaked allocations grouped by stack
SKIP_LEAKS=1 FRAMES=1000 PORT_WRAPPER="heaptrack -o text_leak" \
  xvfb-run -a scripts/repro/text_leak.sh
heaptrack_print --print-leaks 1 text_leak.*.zst | less
```

In both, the text leak is the allocations under
`cairo_scaled_font_text_to_glyphs` called from `script_ops_draw_text`; the
pattern leak is under `_cairo_pattern_create_*` from `script_ops_fill_color`.
