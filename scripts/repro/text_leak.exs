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

alias Scenic.Driver.Local.ToPort
alias Scenic.Script

defmodule TextLeakRepro do
  @msg_ready_id 0x06
  @op_font 0x90
  @font_id "repro_font"
  @warmup_frames 200
  # Generous, because the first frame under valgrind includes GTK start-up.
  @frame_timeout 120_000
  @exit_timeout 300_000

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
          {find_executable!(cmd), wrapper_args ++ [port_path() | args]}
      end

    Port.open({:spawn_executable, exe}, [
      :binary,
      {:packet, 4},
      {:args, args},
      {:env, env}
    ])
  end

  # A bare name is looked up on PATH; anything with a slash is a path.
  defp find_executable!(cmd) do
    exe = if String.contains?(cmd, "/"), do: Path.expand(cmd), else: System.find_executable(cmd)

    if exe && File.regular?(exe) do
      exe
    else
      raise "cannot find executable #{inspect(cmd)}"
    end
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
      @frame_timeout -> raise "timed out waiting for the port to render a frame"
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

  # Ask the port to quit and wait until it has. A port under valgrind or
  # heaptrack writes its report on the way out, so the report is only complete
  # once the port is gone.
  def close_and_wait(port) do
    ToPort.close(port)

    receive do
      {:EXIT, ^port, _reason} -> :ok
    after
      @exit_timeout ->
        close(port)
        raise "timed out waiting for the port to exit"
    end
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

    out |> parse_stacks() |> print_block_rows(total_frames)
    IO.puts("  full leaks output: #{raw}")
  end

  defp print_block_rows(rows, total_frames) do
    IO.puts("  leaked blocks grouped by allocation stack:")
    IO.puts("    blocks  per_frame       bytes  allocated by -> innermost driver frame")

    rows
    |> merge_rows()
    |> Enum.each(fn s ->
      per_frame = Float.round(s.count / total_frames, 1)

      IO.puts(
        "    #{pad(s.count, 6)}  #{pad(per_frame, 9)}  #{pad(s.bytes, 10)}  " <>
          "#{s.alloc} -> #{s.driver_frame}#{label(s)}"
      )
    end)
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
    |> Enum.sort_by(&{-&1.count, -&1.bytes})
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

  # The arrays are allocated by cairo_glyph_allocate/cairo_text_cluster_allocate,
  # which the compiler may inline into cairo_scaled_font_text_to_glyphs.
  defp label(%{alloc: alloc, driver_frame: frame}) do
    cond do
      alloc =~
        ~r/cairo_scaled_font_text_to_glyphs|cairo_glyph_allocate|cairo_text_cluster_allocate/ and
          frame =~ "script_ops_draw_text" ->
        "  [draw_text glyph/cluster arrays]"

      alloc =~ "_cairo_pattern_create" and frame =~ "script_ops_" ->
        "  [pattern per fill/stroke op, separate leak]"

      frame =~ ~r/^main\b/ ->
        "  [GTK main loop, not driver code]"

      true ->
        ""
    end
  end

  # ---------------------------------------------------------------------------
  # Phase 2 on Linux: valgrind memcheck, "definitely lost" records

  def valgrind_phase(n) do
    log_pattern = Path.join(System.tmp_dir!(), "text_leak_repro_valgrind_%p.txt")

    port =
      open_port(
        [],
        ~w(valgrind --leak-check=full --show-leak-kinds=definite --num-callers=40) ++
          ["--log-file=#{log_pattern}"]
      )

    # valgrind runs the port inside its own process, so the pid is the same.
    pid = os_pid(port)
    raw = String.replace(log_pattern, "%p", to_string(pid))
    total_frames = @warmup_frames + n

    IO.puts("")
    IO.puts("valgrind (port pid #{pid}, #{total_frames} frames; slow, please wait)")

    setup(port)
    frames(port, total_frames)
    close_and_wait(port)
    out = File.read!(raw)

    case Regex.run(~r/definitely lost: .*$/m, out) do
      [line] -> IO.puts("  #{line}")
      _ -> IO.puts("  no leak summary line; see #{raw}")
    end

    out |> parse_valgrind() |> print_block_rows(total_frames)
    IO.puts("  full valgrind output: #{raw}")
  end

  # A loss record is a header line followed by the allocation stack, innermost
  # frame first:
  #
  #   ==1== 164,808 bytes in 981 blocks are definitely lost in loss record 9 of 9
  #   ==1==    at 0x484676B: malloc (in /.../vgpreload_memcheck-amd64-linux.so)
  #   ==1==    by 0x59A4C3A: _cairo_pattern_create_solid (in /.../libcairo.so.2)
  #   ==1==    by 0x40C757: script_ops_fill_color (cairo_script_ops.c:703)
  def parse_valgrind(out) do
    sources = driver_sources()

    out
    |> String.replace(~r/^==\d+== ?/m, "")
    |> String.split(~r/\n\s*\n/)
    |> Enum.flat_map(fn record ->
      [header | stack] = String.split(record, "\n")

      case Regex.run(~r/^([\d,]+) .*bytes in ([\d,]+) blocks are definitely lost/, header) do
        [_, bytes, blocks] ->
          frames = Enum.flat_map(stack, &parse_valgrind_frame/1)
          # Drop valgrind's malloc/calloc/realloc replacements.
          frames = Enum.drop_while(frames, fn {_sym, where} -> where =~ "vgpreload" end)
          driver = Enum.drop_while(frames, &(not driver_location?(&1, sources)))

          [
            %{
              count: to_int(blocks),
              bytes: to_int(bytes),
              alloc: frames |> List.first({"?", ""}) |> elem(0),
              driver_frame: driver |> List.first({"(none)", ""}) |> elem(0)
            }
          ]

        _ ->
          []
      end
    end)
  end

  defp parse_valgrind_frame(line) do
    case Regex.run(~r/^\s+(?:at|by) 0x[0-9A-Fa-f]+: (.+) \((in [^()]+|[^()]+:\d+)\)$/, line) do
      [_, sym, where] -> [{sym, where}]
      _ -> []
    end
  end

  # A frame is driver code if valgrind placed it in one of the c_src files
  # (built with -g) or, without debug info, in the port binary.
  defp driver_location?({_sym, where}, sources) do
    String.ends_with?(where, "/scenic_driver_local") or
      MapSet.member?(sources, where |> String.split(":") |> hd())
  end

  defp driver_sources do
    Path.expand("../../c_src/**/*.c", __DIR__)
    |> Path.wildcard()
    |> MapSet.new(&Path.basename/1)
  end

  defp to_int(str), do: str |> String.replace(",", "") |> String.to_integer()

  # ---------------------------------------------------------------------------
  # Phase 2 on Linux: heaptrack, growth in unfreed memory
  #
  # heaptrack reports everything still allocated at exit, which includes font,
  # glyph and GTK caches that are filled once. To leave only what grows with
  # every frame, record two runs (warm-up only, then warm-up plus n frames) and
  # print the difference between them.

  def heaptrack_phase(n) do
    base = Path.join(System.tmp_dir!(), "text_leak_repro_heaptrack_#{System.pid()}")
    baseline = base <> "_baseline.gz"
    raw = base <> ".gz"
    stacks = base <> "_stacks.txt"

    IO.puts("")
    IO.puts("heaptrack (#{@warmup_frames} warm-up frames, then the same plus #{n} frames)")

    heaptrack_record(@warmup_frames, baseline)
    heaptrack_record(@warmup_frames + n, raw)

    {out, status} =
      System.cmd(
        "heaptrack_print",
        ~w(--print-peaks 0 --print-allocators 0 --print-temporary 0) ++
          ~w(--flamegraph-cost-type leaked --print-flamegraph #{stacks}) ++
          ["--diff", baseline, raw],
        stderr_to_stdout: true
      )

    if status != 0, do: raise("heaptrack_print failed:\n#{out}")

    IO.puts("  growth in unfreed bytes over #{n} frames, by allocation stack:")
    IO.puts("         bytes  per_frame  allocated by -> innermost driver frame")

    rows = stacks |> File.read!() |> parse_heaptrack_stacks()
    {shown, rest} = Enum.split(rows, 12)

    Enum.each(shown, fn s ->
      per_frame = Float.round(s.bytes / n, 1)

      IO.puts(
        "    #{pad(s.bytes, 10)}  #{pad(per_frame, 9)}  " <>
          "#{s.alloc} -> #{s.driver_frame}#{label(s)}"
      )
    end)

    if rows == [], do: IO.puts("    (none)")
    if rest != [], do: IO.puts("    ... and #{length(rest)} smaller rows")
    IO.puts("  heaptrack data: #{raw}")
    IO.puts("  baseline:       #{baseline}")
    IO.puts("  full report:    heaptrack_print --print-leaks 1 --diff BASELINE DATA")
  end

  # heaptrack_port.sh rather than `heaptrack`: the stock launcher prints to
  # stdout, which is the port's packet channel.
  defp heaptrack_record(frame_count, output) do
    port = open_port([], [Path.expand("heaptrack_port.sh", __DIR__), output])
    setup(port)
    frames(port, frame_count)
    close_and_wait(port)
  end

  # heaptrack_print's flame graph file has one line per allocation stack,
  # outermost frame first, then the cost (here: growth in bytes never freed):
  #
  #   __clone3;...;render_script (script.c);script_ops_fill_color (cairo_script_ops.c);cairo_pattern_create_rgba;_cairo_pattern_create_solid; 192024
  #
  # Frames with debug info carry their source file, which is how driver frames
  # are recognised. Stacks that didn't grow or have no driver frame are dropped.
  def parse_heaptrack_stacks(text) do
    sources = driver_sources()

    text
    |> String.split("\n", trim: true)
    |> Enum.flat_map(fn line ->
      with [_, stack, cost] <- Regex.run(~r/^(.*) (-?\d+)$/, line),
           frames = stack |> String.split(";", trim: true) |> Enum.reverse(),
           {inner, [driver | _]} <-
             Enum.split_while(frames, &(not heaptrack_driver_frame?(&1, sources))) do
        [
          %{
            bytes: String.to_integer(cost),
            alloc: strip_source(List.first(inner, driver)),
            driver_frame: strip_source(driver)
          }
        ]
      else
        _ -> []
      end
    end)
    |> merge_heaptrack_rows()
    |> Enum.filter(&(&1.bytes > 0))
  end

  defp merge_heaptrack_rows(rows) do
    rows
    |> Enum.group_by(&{&1.alloc, &1.driver_frame})
    |> Enum.map(fn {_, [first | _] = group} ->
      %{first | bytes: group |> Enum.map(& &1.bytes) |> Enum.sum()}
    end)
    |> Enum.sort_by(&(-&1.bytes))
  end

  defp heaptrack_driver_frame?(frame, sources) do
    case Regex.run(~r/ \(([^()]+)\)$/, frame) do
      [_, file] -> MapSet.member?(sources, Path.basename(file))
      _ -> false
    end
  end

  defp strip_source(frame), do: String.replace(frame, ~r/ \([^()]+\)$/, "")

  # ---------------------------------------------------------------------------
  # Choosing the leak tool

  @leak_tools ~w(leaks valgrind heaptrack)

  def leak_tools do
    requested = System.get_env("LEAK_TOOL", "auto")

    cond do
      System.get_env("SKIP_LEAKS") == "1" or requested == "none" ->
        []

      requested == "auto" ->
        case Enum.find(@leak_tools, &System.find_executable/1) do
          nil ->
            IO.puts(
              "\nNo leak tool found (leaks on macOS; valgrind or heaptrack on Linux). " <>
                "See scripts/repro/README.md."
            )

            []

          tool ->
            [tool]
        end

      true ->
        requested |> String.split(",", trim: true) |> Enum.map(&String.trim/1)
    end
  end

  def leak_phase(tool, n) when tool in @leak_tools do
    if System.find_executable(tool) do
      case tool do
        "leaks" -> leaks_phase(n)
        "valgrind" -> valgrind_phase(n)
        "heaptrack" -> heaptrack_phase(n)
      end
    else
      IO.puts("\nLEAK_TOOL=#{tool}: `#{tool}` is not on PATH, skipping.")
    end
  end

  def leak_phase(tool, _n) do
    IO.puts("\nLEAK_TOOL=#{tool}: unknown tool (expected #{Enum.join(@leak_tools, ", ")}).")
  end

  defp pad(v, w), do: v |> to_string() |> String.pad_leading(w)
end

Process.flag(:trap_exit, true)

TextLeakRepro.rss_phase(TextLeakRepro.env_int("FRAMES", 5000))

for tool <- TextLeakRepro.leak_tools() do
  TextLeakRepro.leak_phase(tool, TextLeakRepro.env_int("LEAK_FRAMES", 100))
end
