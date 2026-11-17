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
2. **Leak attribution.** Renders 200 + `LEAK_FRAMES` frames under a heap tool
   and summarises what leaked, grouped by allocation site and innermost driver
   frame, labelled where the stack is recognised. The tool's full output is
   saved to a temp file whose path is printed. `LEAK_TOOL` picks the tool; by
   default it is the first one installed out of:
   - `leaks` (macOS): the port runs with `MallocStackLogging=1` and
     `leaks <pid>` is run against it while it is still alive.
   - `valgrind` (Linux): the port runs under memcheck and the "definitely
     lost" records are summarised once it exits. About 10x slower than
     native.
   - `heaptrack` (Linux): heaptrack reports everything still allocated at
     exit, which includes font, glyph and GTK caches that are filled once. So
     the port is recorded twice, with 200 frames and with 200 + `LEAK_FRAMES`
     frames, and the table shows the difference: memory that grows with
     every frame. Runs at close to native speed.

The script's `fill_color` op also leaks a `cairo_pattern_t` per frame
(a separate bug: every fill/stroke colour or gradient op leaks a pattern). It
is kept in on purpose so the report shows the two leaks apart: text leaks are
allocated in `cairo_scaled_font_text_to_glyphs` under `script_ops_draw_text`,
pattern leaks in `_cairo_pattern_create_*` under `script_ops_fill_color`. RSS
alone can't tell them apart; the grouping by stack can.

## Prerequisites

- Elixir and Erlang matching `mix.exs` (tested with Elixir 1.16.3 / OTP 26).
- macOS: `brew install gtk+3 cairo freetype pkg-config`. `leaks` ships with the
  Xcode command line tools.
- Linux with nix: nothing to install. The flake's dev shell has the build
  dependencies plus `valgrind`, `heaptrack` and `xvfb-run`, and the script
  enters it by itself (see below).
- Linux without nix: `apt install libgtk-3-dev libcairo2-dev libfreetype-dev
  pkg-config valgrind heaptrack xvfb` (or your distro's equivalents).

## Running

```sh
scripts/repro/text_leak.sh
```

The script works out the environment for itself:

- **macOS:** runs natively and uses `leaks`.
- **Linux:** if `mix`, `pkg-config`, `valgrind` or `heaptrack` is missing from
  `PATH` and `nix` is installed, it re-runs itself inside the flake's dev shell
  (`nix develop`), whether or not you are already in a nix shell. Set
  `TEXT_LEAK_NO_NIX=1` to stay in the current environment and use whatever is
  installed there.
- **Linux with no display** (`DISPLAY` and `WAYLAND_DISPLAY` both unset): runs
  under `xvfb-run`.

Settings are environment variables:

| Variable           | Default | Meaning                                                    |
| ------------------ | ------- | ---------------------------------------------------------- |
| `FRAMES`           | 5000    | frames in the RSS phase                                    |
| `TEXTS`            | 20      | `draw_text` calls per frame                                |
| `LEAK_FRAMES`      | 100     | frames after warm-up in the leak phase                     |
| `LEAK_TOOL`        | `auto`  | `leaks`, `valgrind`, `heaptrack`, `none`, or a comma-separated list such as `valgrind,heaptrack` |
| `SKIP_LEAKS`       | unset   | `1` skips the leak phase (same as `LEAK_TOOL=none`)        |
| `PORT_WRAPPER`     | unset   | command the RSS-phase port runs under                      |
| `TEXT_LEAK_NO_NIX` | unset   | `1` stops the script entering the nix dev shell            |

To compare before and after a C change, run it on each commit; the script
removes the built port binary first so the C code is always rebuilt.

## Reading the output

With the defaults, a leaking `draw_text` shows in `leaks` and valgrind as about
40 blocks per frame (one glyph array and one cluster array for each of the 20
calls) from `cairo_scaled_font_text_to_glyphs -> script_ops_draw_text`, and RSS
grows by about 12.8 MB per 1,000 frames. heaptrack shows the same row in bytes:
about 12.5 KB per frame. Without the leak those rows are absent.
The `script_ops_fill_color` pattern row is the separate pattern leak, and a few
patterns allocated by GTK's own window drawing (innermost driver frame `main`)
also show up; those are not driver code.

valgrind also lists two one-off blocks from start-up (`scenic_cairo_init`,
`alloc_font_data`); at one block each they are not per-frame leaks.

## Running the port under another tool

`PORT_WRAPPER` runs the RSS-phase port under any command, for tools or options
the leak phase doesn't cover. The port exits cleanly when the phase ends, so
the tool writes its report then. The RSS column then measures the wrapper's
process, not the port alone.

```sh
SKIP_LEAKS=1 FRAMES=1000 \
  PORT_WRAPPER="valgrind --tool=massif --massif-out-file=massif.out" \
  scripts/repro/text_leak.sh
```

A port's stdout is its packet channel to the BEAM, so a wrapper must not write
to stdout. The stock `heaptrack` launcher does, which breaks the protocol; use
`scripts/repro/heaptrack_port.sh OUTPUT.gz` in its place. It records the same
data and leaves stdout alone:

```sh
SKIP_LEAKS=1 FRAMES=1000 \
  PORT_WRAPPER="scripts/repro/heaptrack_port.sh text_leak.gz" \
  scripts/repro/text_leak.sh
heaptrack_print --print-leaks 1 text_leak.gz | less
```
