# Reproduce the cairo draw_text glyph/cluster array leak.
#
# Drives the real scenic_driver_local port binary over its port protocol (no
# ViewPort): loads a font, puts a "_root_" script that calls draw_text many
# times, then sends `render` and waits for the port's `ready` reply, once per
# frame. It reports RSS growth per 1,000 frames and, on macOS, groups the
# `leaks` output by allocation stack so the text leak can be told apart from
# other per-frame leaks.
#
# Run through scripts/repro/text_leak.sh, which builds the cairo-gtk binary
# first. Settings come from environment variables:
#
#   FRAMES        frames in the RSS phase              (default 5000)
#   TEXTS         draw_text calls per frame            (default 20)
#   LEAK_FRAMES   frames in the `leaks` phase          (default 100)
#   SKIP_LEAKS=1  skip the `leaks` phase
#   PORT_WRAPPER  command to run the port under in the RSS phase, e.g.
#                 "valgrind --leak-check=full --log-file=vg.%p.txt"

alias Scenic.Driver.Local.ToPort
alias Scenic.Script

defmodule TextLeakRepro do
  @msg_ready_id 0x06
  @op_font 0x90
  @font_id "repro_font"
  @warmup_frames 200

  def env_int(name, default) do
    case System.get_env(name) do
      nil -> default
      val -> String.to_integer(val)
    end
  end

  def port_path do
    Path.join(:code.priv_dir(:scenic_driver_local), "scenic_driver_local")
  end

  # Same argument order as Scenic.Driver.Local.init/2:
  # cursor layer opacity antialias debug debug_fps width height resizeable fbdev title
  def open_port(env, wrapper \\ []) do
    args = ~w(0 0 255 1 0 0 400 #{texts() * 22 + 20} 0 /dev/fb0 text_leak_repro)

    {exe, args} =
      case wrapper do
        [] ->
          {port_path(), args}

        [cmd | wrapper_args] ->
          {System.find_executable(cmd), wrapper_args ++ [port_path() | args]}
      end

    Port.open({:spawn_executable, exe}, [
      :binary,
      {:packet, 4},
      {:args, args},
      {:env, env}
    ])
  end

  def texts, do: env_int("TEXTS", 20)

  # One fill colour, one font, then TEXTS draw_text calls. The fill_color op
  # also leaks a cairo_pattern_t every frame (a separate bug); it is kept in
  # so the leak report shows the two apart.
  def root_script do
    ops =
      Script.start()
      |> Script.fill_color(:white)
      |> Script.font_size(18)
      # Script.font/2 resolves ids through a Scenic static asset library, which
      # this script doesn't have, so add the op directly; serialize_font/1
      # encodes it with the name used in put_font.
      |> then(&[{:font, @font_id} | &1])

    1..texts()
    |> Enum.reduce(ops, fn i, ops ->
      ops
      |> Script.translate(0, 22)
      |> Script.draw_text("Hello world, line #{i}")
    end)
    |> Script.finish()
    |> Script.serialize(&serialize_font/1)
  end

  defp serialize_font({:font, id}) do
    [<<@op_font::16-big, byte_size(id)::16-big>>, Script.padded_string(id)]
  end

  defp serialize_font(op), do: op

  def setup(port) do
    font = File.read!(Path.expand("../../deps/scenic/assets/fonts/roboto.ttf", __DIR__))
    ToPort.put_font(port, @font_id, font)
    ToPort.clear_color(:black, port)
    ToPort.put_script(root_script(), "_root_", port)
  end

  def frame(port) do
    ToPort.render(port)
    await_ready(port)
  end

  defp await_ready(port) do
    receive do
      {^port, {:data, <<@msg_ready_id::unsigned-integer-size(32)-native>>}} -> :ok
      {^port, {:data, _other}} -> await_ready(port)
      {:EXIT, ^port, reason} -> raise "port exited: #{inspect(reason)}"
    after
      10_000 -> raise "timed out waiting for the port to render a frame"
    end
  end

  def frames(port, n), do: Enum.each(1..n, fn _ -> frame(port) end)

  def os_pid(port) do
    {:os_pid, pid} = Port.info(port, :os_pid)
    pid
  end

  def rss_kb(pid) do
    {out, 0} = System.cmd("ps", ["-o", "rss=", "-p", to_string(pid)])
    out |> String.trim() |> String.to_integer()
  end

  def close(port) do
    ToPort.close(port)
    Port.close(port)
  catch
    _, _ -> :ok
  end

  # ---------------------------------------------------------------------------
  # Phase 1: RSS growth

  def rss_phase(total) do
    port = open_port([], OptionParser.split(System.get_env("PORT_WRAPPER", "")))
    pid = os_pid(port)
    setup(port)
    frames(port, @warmup_frames)

    IO.puts("RSS (port pid #{pid}, #{texts()} draw_text calls/frame, #{total} frames)")
    IO.puts("  frames      rss_kb   delta_kb_per_1000")

    start = rss_kb(pid)
    IO.puts("  #{pad(0, 6)}  #{pad(start, 10)}")

    last =
      Enum.reduce(1..div(total, 1000), start, fn i, prev ->
        frames(port, 1000)
        now = rss_kb(pid)
        IO.puts("  #{pad(i * 1000, 6)}  #{pad(now, 10)}  #{pad(now - prev, 18)}")
        now
      end)

    IO.puts(
      "  average growth: #{Float.round((last - start) / max(div(total, 1000), 1), 1)} KB per 1,000 frames"
    )

    close(port)
  end

  # ---------------------------------------------------------------------------
  # Phase 2: leaked blocks grouped by allocation stack (macOS `leaks`)

  def leaks_phase(n) do
    port = open_port([{~c"MallocStackLogging", ~c"1"}])
    pid = os_pid(port)
    setup(port)
    frames(port, @warmup_frames + n)

    {out, _status} = System.cmd("leaks", [to_string(pid)], stderr_to_stdout: true)
    raw = Path.join(System.tmp_dir!(), "text_leak_repro_leaks_#{pid}.txt")
    File.write!(raw, out)
    close(port)

    total_frames = @warmup_frames + n
    IO.puts("")
    IO.puts("leaks (port pid #{pid}, #{total_frames} frames, MallocStackLogging=1)")

    case Regex.run(~r/Process \d+: \d+ leaks? for \d+ total leaked bytes/, out) do
      [line] -> IO.puts("  #{line}")
      _ -> IO.puts("  no leak summary line; see #{raw}")
    end

    IO.puts("  leaked blocks grouped by allocation stack:")
    IO.puts("    blocks  per_frame       bytes  allocated by -> innermost driver frame")

    out
    |> parse_stacks()
    |> merge_rows()
    |> Enum.each(fn s ->
      per_frame = Float.round(s.count / total_frames, 1)

      IO.puts(
        "    #{pad(s.count, 6)}  #{pad(per_frame, 9)}  #{pad(s.bytes, 10)}  " <>
          "#{s.alloc} -> #{s.driver_frame}#{label(s)}"
      )
    end)

    IO.puts("  full leaks output: #{raw}")
  end

  # With MallocStackLogging on, `leaks` merges leaks with an identical stack
  # into one "STACK OF n INSTANCES OF 'ROOT LEAK: ...'" block: the stack
  # (outermost frame first, frame 0 last), "====", then one line per leaked
  # block ending in "[size]".
  def parse_stacks(out) do
    out
    |> String.split(~r/^STACK OF /m)
    |> Enum.drop(1)
    |> Enum.flat_map(fn block ->
      with [_, count] <- Regex.run(~r/^(\d+) INSTANCES? OF 'ROOT LEAK/, block),
           [stack, leaks] <- String.split(block, "\n====\n", parts: 2) do
        frames = stack |> String.split("\n") |> Enum.drop(1) |> Enum.map(&parse_frame/1)

        [
          %{
            count: String.to_integer(count),
            bytes:
              ~r/ROOT LEAK: .*\[(\d+)\]$/m
              |> Regex.scan(leaks)
              |> Enum.map(fn [_, b] -> String.to_integer(b) end)
              |> Enum.sum(),
            alloc: alloc_site(frames),
            driver_frame: driver_frame(frames)
          }
        ]
      else
        _ -> []
      end
    end)
  end

  # Merge stacks that differ only above the driver (e.g. GTK event paths).
  defp merge_rows(stacks) do
    stacks
    |> Enum.group_by(&{&1.alloc, &1.driver_frame})
    |> Enum.map(fn {_, [first | _] = group} ->
      %{
        first
        | count: group |> Enum.map(& &1.count) |> Enum.sum(),
          bytes: group |> Enum.map(& &1.bytes) |> Enum.sum()
      }
    end)
    |> Enum.sort_by(&(-&1.count))
  end

  # "2   scenic_driver_local   0x102e32cc4 script_ops_draw_text + 152  cairo_script_ops.c:274"
  defp parse_frame(line) do
    case Regex.run(~r/^\s*\d+\s+(\S+)\s+0x[0-9a-f]+\s+(.*?)\s*$/, line) do
      [_, image, symbol] -> {image, symbol}
      _ -> {"?", String.trim(line)}
    end
  end

  # The first frame above malloc/calloc.
  defp alloc_site(frames) do
    frames
    |> Enum.reverse()
    |> Enum.find_value("?", fn {image, sym} ->
      if image != "libsystem_malloc.dylib", do: sym
    end)
  end

  defp driver_frame(frames) do
    frames
    |> Enum.reverse()
    |> Enum.find_value("(none)", fn {image, sym} ->
      if image == "scenic_driver_local", do: sym
    end)
  end

  defp label(%{alloc: alloc, driver_frame: frame}) do
    cond do
      alloc =~ "cairo_scaled_font_text_to_glyphs" and frame =~ "script_ops_draw_text" ->
        "  [draw_text glyph/cluster arrays]"

      alloc =~ "_cairo_pattern_create" and frame =~ "script_ops_" ->
        "  [pattern per fill/stroke op, separate leak]"

      frame =~ ~r/^main\b/ ->
        "  [GTK main loop, not driver code]"

      true ->
        ""
    end
  end

  defp pad(v, w), do: v |> to_string() |> String.pad_leading(w)
end

Process.flag(:trap_exit, true)

TextLeakRepro.rss_phase(TextLeakRepro.env_int("FRAMES", 5000))

cond do
  System.get_env("SKIP_LEAKS") == "1" ->
    :ok

  System.find_executable("leaks") ->
    TextLeakRepro.leaks_phase(TextLeakRepro.env_int("LEAK_FRAMES", 100))

  true ->
    IO.puts(
      "\n`leaks` not found (not macOS); see scripts/repro/README.md for valgrind/heaptrack."
    )
end
