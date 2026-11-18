# Leak reproduction scripts (cairo)

Two scripts that build the real driver for a cairo backend (`cairo-gtk` by
default, or `cairo-fb`), start the port binary directly and talk to it over
the normal port protocol (no ViewPort), then measure what it leaks:

- `leak_sweep.sh` exercises every script op and port command, one at a time,
  and reports which of them leak.
- `text_leak.sh` reproduces one leak, in `draw_text`, in more detail.

Both wait for the port's `ready` reply after every `render`, so every frame is
actually drawn.

## Prerequisites

- Elixir and Erlang matching `mix.exs` (tested with Elixir 1.16.3 / OTP 26).
- macOS: `brew install gtk+3 cairo freetype pkg-config`. `leaks` ships with the
  Xcode command line tools.
- Linux with nix: nothing to install. The flake's dev shell has the build
  dependencies plus `valgrind`, `heaptrack` and `xvfb-run`, and the scripts
  enter it by themselves (see below).
- Linux without nix: `apt install libgtk-3-dev libcairo2-dev libfreetype-dev
  pkg-config valgrind heaptrack xvfb` (or your distro's equivalents).

## Environment

The scripts work out the environment for themselves:

- **macOS:** run natively and use `leaks`. The port's window opens on the
  desktop.
- **Linux:** if `mix`, `pkg-config`, `valgrind` or `heaptrack` is missing from
  `PATH` and `nix` is installed, they re-run themselves inside the flake's dev
  shell (`nix develop`), whether or not you are already in a nix shell. Set
  `REPRO_NO_NIX=1` to stay in the current environment and use whatever is
  installed there.
- **Linux display:** the port runs on a virtual display (`xvfb-run`), so its
  windows neither appear on your desktop nor take input focus. Set
  `REPRO_SHOW_WINDOW=1` to use the real display.

The port's stderr (GTK and fontconfig warnings, messages from the heap tools)
goes to a log file in the temp directory, not the terminal.

## Testing the framebuffer backend

`REPRO_TARGET=cairo-fb` builds and runs the `cairo-fb` backend. It draws
straight into a framebuffer device, so give it one that is nobody's screen:
the kernel's virtual framebuffer.

```sh
sudo modprobe vfb vfb_enable=1        # creates a new /dev/fbN
grep . /sys/class/graphics/fb*/name   # the one named "Virtual FB"
sudo chmod a+rw /dev/fb1              # unless you are in the video group

REPRO_TARGET=cairo-fb REPRO_FBDEV=/dev/fb1 scripts/repro/leak_sweep.sh
```

`REPRO_FBDEV` is required, and the scripts refuse a device that isn't a
virtual framebuffer unless `REPRO_ALLOW_REAL_FB=1` is set. The backend is
built with `-g` here (the Makefile leaves it out for this target) because the
leak tools need debug info to recognise driver frames.

The virtual framebuffer starts at 640x480 and 8 bits per pixel, which runs
the driver's colour-map path. The driver also handles 16, 24 and 32 bits;
changing the device's mode takes an `FBIOPUT_VSCREENINFO` ioctl (`fbset`,
where available). Its default 1 MB of video memory fits 640x480 at 16 bits or
400x300 at 32.

## Leak tools

`LEAK_TOOL` picks the tool: `leaks`, `valgrind`, `heaptrack`, `none`, or a
comma-separated list such as `valgrind,heaptrack`. The default, `auto`, uses
the first one installed.

- `leaks` (macOS): the port runs with `MallocStackLogging=1` and `leaks <pid>`
  is run against it while it is still alive.
- `valgrind` (Linux): the port runs under memcheck and the "definitely lost"
  records are summarised once it exits. About 10x slower than native.
- `heaptrack` (Linux): heaptrack reports everything still allocated at exit,
  which includes font, glyph and GTK caches that are filled once. So the port
  is recorded twice, with a warm-up only and with the warm-up plus the
  measured iterations, and the result is the difference: memory that grows
  with every iteration. Runs at close to native speed.

All three group what leaked by allocation site and innermost driver frame
(`allocated by -> innermost driver frame`). Allocations whose innermost driver
frame is `main` belong to GTK's own event loop, not to driver code.

None of them can see memory that grows while the port runs but is released
when it exits. The image table is the one known case: the driver frees every
image on shutdown.

## leak_sweep.sh

```sh
scripts/repro/leak_sweep.sh
```

Each case exercises one op or command in isolation, in its own port process:

- **Script ops** (`state`, `paint`, `draw`, `path`, `text` groups): a
  `_root_` script containing the op, rendered once per iteration.
- **Port commands** (`command` group): the command is sent once per
  iteration, followed by a render. These include `put_script`, `del_script`,
  `reset`, `put_font`, `put_texture` in every pixel format, and malformed
  input such as a corrupt font or an image of the wrong size.

Draw ops need a fill and a stroke to draw with. The cases set both to an
image, not a colour, because the colour and gradient ops are themselves cases;
that keeps every case's result about its own op.

| Variable     | Default | Meaning                                                    |
| ------------ | ------- | ---------------------------------------------------------- |
| `CASES`      | all     | only cases whose name contains, or whose group is, one of these comma-separated strings, e.g. `paint,draw_text` |
| `ITERATIONS` | 200     | measured iterations per case                               |
| `WARMUP`     | 50      | iterations before measuring starts                         |
| `MIN_BYTES`  | 16      | heaptrack only: growth below this many bytes per iteration at one allocation site is ignored |
| `LEAK_TOOL`  | `auto`  | as above; `auto` prefers `heaptrack` to `valgrind` here    |
| `LIST`       | unset   | `1` prints the case names and exits                        |

The full sweep is 80 cases and takes about 5 minutes with heaptrack. With
valgrind allow about 8 seconds per case at `ITERATIONS=50`.

### Reading the output

One line per case:

```
  fill_color                          168.0  LEAK: _cairo_pattern_create_solid -> script_ops_fill_color
  draw_rect                               0  ok
  put_font_new_id                   41028.1  grows by design: no command deletes a font
  put_texture_file_corrupt                -  FAILED: port died: segmentation fault
```

- `LEAK`: the case leaks that many bytes per iteration, at the sites listed.
- `grows by design`: memory grows, but only because the protocol has no way to
  release it. Listed so the cost is visible.
- `FAILED`: the port crashed or hung, so nothing could be measured. A crash is
  a finding in its own right.

After the cases comes a list of every leaking allocation site with the cases
it appeared in, then the cases that failed, then where the tools' full output
for the leaking cases was kept.

To compare before and after a C change, run it on each commit; the scripts
remove the built port binary first so the C code is always rebuilt.

## text_leak.sh

```sh
scripts/repro/text_leak.sh
```

Checks `script_ops_draw_text` for a per-frame leak.
`cairo_scaled_font_text_to_glyphs` returns a glyph array and a cluster array
that the caller owns; if `draw_text` doesn't free them, every call leaks both.

The port gets a font, a clear colour and a `_root_` script that sets a fill
colour and font and then calls `draw_text` `TEXTS` times. It runs two phases,
each in a fresh port process:

1. **RSS.** 200 warm-up frames, then `FRAMES` frames, printing the port's RSS
   (from `ps`) every 1,000 frames and the average growth per 1,000 frames.
2. **Leak attribution.** 200 warm-up frames plus `LEAK_FRAMES` frames under
   the leak tool.

The script's `fill_color` op also leaks a `cairo_pattern_t` per frame (a
separate bug: every fill/stroke colour or gradient op leaks a pattern). It is
kept in on purpose so the report shows the two leaks apart. RSS alone can't
tell them apart; the grouping by stack can.

| Variable       | Default | Meaning                                              |
| -------------- | ------- | ---------------------------------------------------- |
| `FRAMES`       | 5000    | frames in the RSS phase                              |
| `TEXTS`        | 20      | `draw_text` calls per frame                          |
| `LEAK_FRAMES`  | 100     | frames after warm-up in the leak phase               |
| `LEAK_TOOL`    | `auto`  | as above                                             |
| `SKIP_LEAKS`   | unset   | `1` skips the leak phase (same as `LEAK_TOOL=none`)  |
| `PORT_WRAPPER` | unset   | command the RSS-phase port runs under                |

With the defaults, a leaking `draw_text` shows in `leaks` and valgrind as about
40 blocks per frame (one glyph array and one cluster array for each of the 20
calls) from `cairo_scaled_font_text_to_glyphs -> script_ops_draw_text`, and RSS
grows by about 12.8 MB per 1,000 frames. heaptrack shows the same row in bytes:
about 12.5 KB per frame. Without the leak those rows are absent.

valgrind also lists two one-off blocks from start-up (`scenic_cairo_init`,
`alloc_font_data`); at one block each they are not per-frame leaks.

### Running the port under another tool

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
`scripts/repro/support/heaptrack_port.sh OUTPUT.gz` in its place. It records
the same data and leaves stdout alone:

```sh
SKIP_LEAKS=1 FRAMES=1000 \
  PORT_WRAPPER="scripts/repro/support/heaptrack_port.sh text_leak.gz" \
  scripts/repro/text_leak.sh
heaptrack_print --print-leaks 1 text_leak.gz | less
```

## Layout

- `run.sh`: builds the driver and runs one script; the two `.sh` entry points
  call it.
- `support/harness.exs`: starting and driving the port, and the three leak
  tools.
- `support/heaptrack_port.sh`: runs a port under heaptrack.
