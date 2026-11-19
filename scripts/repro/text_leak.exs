# Reproduce the cairo draw_text glyph/cluster array leak.
#
# Drives the real scenic_driver_local port binary over its port protocol (no
# ViewPort): loads a font, puts a "_root_" script that calls draw_text many
# times, then sends `render` and waits for the port's `ready` reply, once per
# frame. It reports RSS growth per 1,000 frames and then groups leaked memory
# by allocation stack, so the text leak can be told apart from other per-frame
# leaks. The stacks come from `leaks` on macOS and from valgrind or heaptrack
# on Linux.
#
# Run through scripts/repro/text_leak.sh, which builds the cairo-gtk binary
# first. Settings come from environment variables:
#
#   FRAMES        frames in the RSS phase              (default 5000)
#   TEXTS         draw_text calls per frame            (default 20)
#   LEAK_FRAMES   frames in the leak phase             (default 100)
#   LEAK_TOOL     auto | leaks | valgrind | heaptrack | none, or a comma
#                 separated list such as "valgrind,heaptrack" (default auto:
#                 the first of leaks, valgrind, heaptrack that is installed)
#   SKIP_LEAKS=1  skip the leak phase (same as LEAK_TOOL=none)
#   PORT_WRAPPER  command to run the port under in the RSS phase, e.g.
#                 "valgrind --leak-check=full --log-file=vg.%p.txt"

Code.require_file("support/harness.exs", __DIR__)

alias Scenic.Driver.Local.ToPort
alias Scenic.Script

defmodule TextLeakRepro do
  @font_id "repro_font"
  @warmup_frames 200

  def texts, do: Repro.env_int("TEXTS", 20)

  def window_size, do: {400, texts() * 22 + 20}

  # One fill colour, one font, then TEXTS draw_text calls. The fill_color op
  # also leaks a cairo_pattern_t every frame (a separate bug); it is kept in
  # so the leak report shows the two apart.
  def root_script do
    ops =
      Script.start()
      |> Script.fill_color(:white)
      |> Script.font_size(18)
      |> then(&[{:font, @font_id} | &1])

    1..texts()
    |> Enum.reduce(ops, fn i, ops ->
      ops
      |> Script.translate(0, 22)
      |> Script.draw_text("Hello world, line #{i}")
    end)
    |> Repro.serialize()
  end

  def setup(port) do
    ToPort.put_font(port, @font_id, Repro.roboto())
    ToPort.clear_color(:black, port)
    ToPort.put_script(root_script(), "_root_", port)
  end

  # ---------------------------------------------------------------------------
  # Phase 1: RSS growth

  def rss_phase(total) do
    wrapper = OptionParser.split(System.get_env("PORT_WRAPPER", ""))
    port = Repro.open_port([], wrapper, window_size())
    pid = Repro.os_pid(port)
    setup(port)
    Repro.frames(port, @warmup_frames)

    IO.puts("RSS (port pid #{pid}, #{texts()} draw_text calls/frame, #{total} frames)")
    IO.puts("  frames      rss_kb   delta_kb_per_1000")

    start = Repro.rss_kb(pid)
    IO.puts("  #{Repro.pad(0, 6)}  #{Repro.pad(start, 10)}")

    last =
      Enum.reduce(1..div(total, 1000)//1, start, fn i, prev ->
        Repro.frames(port, 1000)
        now = Repro.rss_kb(pid)

        IO.puts(
          "  #{Repro.pad(i * 1000, 6)}  #{Repro.pad(now, 10)}  #{Repro.pad(now - prev, 18)}"
        )

        now
      end)

    IO.puts(
      "  average growth: #{Float.round((last - start) / max(div(total, 1000), 1), 1)} KB per 1,000 frames"
    )

    Repro.close(port)
  end

  # ---------------------------------------------------------------------------
  # Phase 2: leaked memory grouped by allocation stack

  def leak_phase(tool, n) do
    IO.puts("")

    IO.puts(
      case tool do
        "leaks" ->
          "leaks (#{@warmup_frames + n} frames, MallocStackLogging=1)"

        "valgrind" ->
          "valgrind (#{@warmup_frames + n} frames; slow, please wait)"

        "heaptrack" ->
          "heaptrack (#{@warmup_frames} warm-up frames, then the same plus #{n} frames)"
      end
    )

    # The harness renders a frame after every step, so the step is empty.
    workload = %{
      setup: &setup/1,
      step: fn _port, _i -> :ok end,
      size: window_size()
    }

    base = Path.join(System.tmp_dir!(), "text_leak_repro_#{System.pid()}")

    tool
    |> Repro.measure(workload, @warmup_frames, n, base)
    |> Repro.print_measurement()
  end
end

Process.flag(:trap_exit, true)

TextLeakRepro.rss_phase(Repro.env_int("FRAMES", 5000))

for tool <- Repro.leak_tools() do
  TextLeakRepro.leak_phase(tool, Repro.env_int("LEAK_FRAMES", 100))
end
