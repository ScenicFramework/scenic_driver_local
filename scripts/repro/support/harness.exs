# Shared code for the scripts in scripts/repro: starting the real
# scenic_driver_local port binary, driving it over its port protocol, and
# measuring what it leaks with `leaks` (macOS), valgrind or heaptrack (Linux).
#
# Load it with Code.require_file("support/harness.exs", __DIR__).

defmodule Repro do
  alias Scenic.Driver.Local.ToPort
  alias Scenic.Script

  @msg_ready_id 0x06
  @op_font 0x90
  @op_fill_image 0x63
  @op_stroke_image 0x74
  @op_draw_sprites 0x0B

  # Generous, because the first frame under valgrind includes GTK start-up.
  @frame_timeout 120_000
  @quit_timeout 10_000
  @exit_timeout 60_000

  # The driver starts reading commands before GTK's main loop is running. A
  # quit that arrives before the loop starts is lost (gtk_main_quit does
  # nothing), and the port then never exits. Short runs can finish inside that
  # window, so a port is kept alive at least this long before it is told to quit.
  @min_lifetime 1_500

  @leak_tools ~w(leaks valgrind heaptrack)

  def env_int(name, default) do
    case System.get_env(name) do
      nil -> default
      val -> String.to_integer(val)
    end
  end

  # ---------------------------------------------------------------------------
  # The port

  def port_path do
    Path.join(:code.priv_dir(:scenic_driver_local), "scenic_driver_local")
  end

  # Same argument order as Scenic.Driver.Local.init/2:
  # cursor layer opacity antialias debug debug_fps width height resizeable fbdev title
  #
  # `wrapper` is a command (list of words) to run the port under.
  def open_port(env \\ [], wrapper \\ [], {width, height} \\ {400, 300}) do
    # Only the cairo-fb target opens the framebuffer device; run.sh sets
    # REPRO_FBDEV for it after checking the device is a virtual one.
    fbdev = System.get_env("REPRO_FBDEV", "/dev/fb0")
    args = ~w(0 0 255 1 0 0 #{width} #{height} 0 #{fbdev} scenic_repro)

    {exe, args} =
      case wrapper do
        [] ->
          {port_path(), args}

        [cmd | wrapper_args] ->
          {find_executable!(cmd), wrapper_args ++ [port_path() | args]}
      end

    # stderr carries only library chatter (GTK, fontconfig) and the heap
    # tools' own messages; the driver reports its errors over the port
    # protocol. Send it to a log file so it doesn't bury the report.
    port =
      Port.open({:spawn_executable, "/bin/sh"}, [
        :binary,
        :exit_status,
        {:packet, 4},
        {:args, ["-c", ~s(exec "$@" 2>>"$0"), stderr_log(), exe | args]},
        {:env, env}
      ])

    Process.put({__MODULE__, port}, {System.monotonic_time(:millisecond), os_pid(port)})
    port
  end

  @doc "The file the ports' stderr is appended to."
  def stderr_log do
    Path.join(System.tmp_dir!(), "scenic_repro_#{System.pid()}_port_stderr.log")
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

  # Render one frame and wait for the port to say it is done.
  def frame(port) do
    ToPort.render(port)
    await_ready(port)
  end

  defp await_ready(port) do
    receive do
      {^port, {:data, <<@msg_ready_id::unsigned-integer-size(32)-native>>}} -> :ok
      {^port, {:data, _other}} -> await_ready(port)
      {^port, {:exit_status, status}} -> raise "port died: #{describe_status(status)}"
      {:EXIT, ^port, reason} -> raise "port exited: #{inspect(reason)}"
    after
      @frame_timeout -> raise "timed out waiting for the port to render a frame"
    end
  end

  # A process killed by signal N is reported as status 128 + N.
  defp describe_status(139), do: "segmentation fault"
  defp describe_status(134), do: "aborted"
  defp describe_status(status) when status > 128, do: "killed by signal #{status - 128}"
  defp describe_status(status), do: "exit status #{status}"

  def frames(port, n), do: Enum.each(1..n//1, fn _ -> frame(port) end)

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
  #
  # Returns :ok, or :terminated if the port ignored the quit and had to be
  # sent SIGTERM (see @min_lifetime; start-up under valgrind can outlast it).
  def close_and_wait(port) do
    {started, _pid} = Process.get({__MODULE__, port})
    Process.sleep(max(started + @min_lifetime - System.monotonic_time(:millisecond), 0))
    ToPort.close(port)

    with :timeout <- await_exit(port, @quit_timeout),
         signal(port, "TERM"),
         :timeout <- await_exit(port, @exit_timeout) do
      kill(port)
      raise "timed out waiting for the port to exit"
    else
      {:exited, 0} -> :ok
      {:exited, 143} -> :terminated
      {:exited, status} -> raise "port died: #{describe_status(status)}"
    end
  end

  defp await_exit(port, timeout) do
    receive do
      {^port, {:exit_status, status}} -> {:exited, status}
      {:EXIT, ^port, _reason} -> {:exited, 0}
    after
      timeout -> :timeout
    end
  end

  # Signal a port's OS process, and the program it wraps if it is a wrapper.
  defp signal(port, name) do
    case Process.get({__MODULE__, port}) do
      {_started, pid} ->
        System.cmd("pkill", ["-#{name}", "-P", to_string(pid)], stderr_to_stdout: true)
        System.cmd("kill", ["-#{name}", to_string(pid)], stderr_to_stdout: true)

      nil ->
        :ok
    end
  end

  @doc "Kill a port's OS process, and the program it wraps if it is a wrapper."
  def kill(port) do
    signal(port, "KILL")
    Process.delete({__MODULE__, port})
    close(port)
  end

  @doc "Kill every port this process opened that is still running."
  def kill_all do
    for {{__MODULE__, port}, _} <- Process.get(), is_port(port) do
      if Port.info(port), do: kill(port), else: Process.delete({__MODULE__, port})
    end

    :ok
  end

  # ---------------------------------------------------------------------------
  # Scripts
  #
  # Scenic.Script resolves font and image ids through a static asset library,
  # which these scripts don't have. Add those ops to a script as plain tuples
  # ({:font, id}, {:fill_image, id}, {:stroke_image, id},
  # {:draw_sprites, {id, cmds}}) and serialize with this; they are encoded
  # with the id exactly as given, i.e. the name used in put_font/put_texture.

  def serialize(ops) do
    ops |> Script.finish() |> Script.serialize(&serialize_op/1)
  end

  defp serialize_op({:font, id}), do: id_op(@op_font, id)
  defp serialize_op({:fill_image, id}), do: id_op(@op_fill_image, id)
  defp serialize_op({:stroke_image, id}), do: id_op(@op_stroke_image, id)

  defp serialize_op({:draw_sprites, {id, cmds}}) do
    [
      <<@op_draw_sprites::16-big, byte_size(id)::16-big, length(cmds)::32-big>>,
      Script.padded_string(id),
      for {{sx, sy}, {sw, sh}, {dx, dy}, {dw, dh}, alpha} <- cmds do
        for v <- [sx, sy, sw, sh, dx, dy, dw, dh, alpha], do: <<v::float-32-big>>
      end
    ]
  end

  defp serialize_op(op), do: op

  defp id_op(op, id) do
    [<<op::16-big, byte_size(id)::16-big>>, Script.padded_string(id)]
  end

  def roboto do
    File.read!(Path.expand("../../../deps/scenic/assets/fonts/roboto.ttf", __DIR__))
  end

  # ---------------------------------------------------------------------------
  # Workloads and measurement
  #
  # A workload is %{setup: (port -> any), step: (port, i -> any)}, with an
  # optional size: {width, height} for the window. It is run as setup, then
  # `count` iterations of step followed by one rendered frame.

  defp size(workload), do: Map.get(workload, :size, {400, 300})

  def run_workload(port, workload, count) do
    workload.setup.(port)

    Enum.each(1..count//1, fn i ->
      workload.step.(port, i)
      frame(port)
    end)
  end

  @doc """
  The tools to use, from LEAK_TOOL / SKIP_LEAKS. `auto` picks the first of
  `order` that is installed.
  """
  def leak_tools(order \\ @leak_tools) do
    requested = System.get_env("LEAK_TOOL", "auto")

    cond do
      System.get_env("SKIP_LEAKS") == "1" or requested == "none" ->
        []

      requested == "auto" ->
        case Enum.find(order, &System.find_executable/1) do
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
        requested
        |> String.split(",", trim: true)
        |> Enum.map(&String.trim/1)
        |> Enum.filter(fn tool ->
          cond do
            tool not in @leak_tools ->
              IO.puts(
                "\nLEAK_TOOL=#{tool}: unknown tool (expected #{Enum.join(@leak_tools, ", ")})."
              )

              false

            System.find_executable(tool) == nil ->
              IO.puts("\nLEAK_TOOL=#{tool}: `#{tool}` is not on PATH, skipping.")
              false

            true ->
              true
          end
        end)
    end
  end

  @doc """
  Run `workload` under `tool` and return what leaked:

      %{
        tool: "valgrind",
        rows: [%{bytes:, count:, alloc:, driver_frame:}],  # largest first
        iterations: 250,   # what the rows' totals are spread over
        summary: "definitely lost: ..." | nil,
        files: [{"full valgrind output", path}]
      }

  `leaks` and valgrind count leaked blocks over all `warmup + n` iterations.
  heaptrack has no notion of a leak, only of memory not freed at exit, so it
  is run twice (`warmup`, then `warmup + n` iterations) and the rows are the
  growth between the two; `count` is nil.

  `base` is a path prefix for the files the tool writes.
  """
  def measure(tool, workload, warmup, n, base)

  # With MallocStackLogging on, `leaks` can show where each block was allocated.
  def measure("leaks", workload, warmup, n, base) do
    port = open_port([{~c"MallocStackLogging", ~c"1"}], [], size(workload))
    pid = os_pid(port)
    run_workload(port, workload, warmup + n)

    {out, _status} = System.cmd("leaks", [to_string(pid)], stderr_to_stdout: true)
    raw = base <> "_leaks.txt"
    File.write!(raw, out)
    close(port)

    %{
      tool: "leaks",
      rows: out |> parse_leaks() |> merge_rows(),
      iterations: warmup + n,
      summary:
        case Regex.run(~r/Process \d+: \d+ leaks? for \d+ total leaked bytes/, out) do
          [line] -> line
          _ -> nil
        end,
      files: [{"full leaks output", raw}]
    }
  end

  def measure("valgrind", workload, warmup, n, base) do
    raw = base <> "_valgrind.txt"

    port =
      open_port(
        [],
        ~w(valgrind --leak-check=full --show-leak-kinds=definite --num-callers=40) ++
          ["--log-file=#{raw}"],
        size(workload)
      )

    run_workload(port, workload, warmup + n)
    close_and_wait(port)
    out = File.read!(raw)

    %{
      tool: "valgrind",
      rows: out |> parse_valgrind() |> merge_rows(),
      iterations: warmup + n,
      summary:
        case Regex.run(~r/definitely lost: .*$/m, out) do
          [line] -> line
          _ -> nil
        end,
      files: [{"full valgrind output", raw}]
    }
  end

  def measure("heaptrack", workload, warmup, n, base) do
    baseline = base <> "_heaptrack_baseline.gz"
    raw = base <> "_heaptrack.gz"
    stacks = base <> "_heaptrack_stacks.txt"

    heaptrack_record(workload, warmup, baseline)
    heaptrack_record(workload, warmup + n, raw)

    {out, status} =
      System.cmd(
        "heaptrack_print",
        ~w(--print-peaks 0 --print-allocators 0 --print-temporary 0) ++
          ~w(--flamegraph-cost-type leaked --print-flamegraph #{stacks}) ++
          ["--diff", baseline, raw],
        stderr_to_stdout: true
      )

    if status != 0, do: raise("heaptrack_print failed:\n#{out}")

    %{
      tool: "heaptrack",
      rows: stacks |> File.read!() |> parse_heaptrack_stacks(),
      iterations: n,
      summary: nil,
      files: [
        {"heaptrack data", raw},
        {"baseline", baseline},
        {"full report", "heaptrack_print --print-leaks 1 --diff BASELINE DATA"}
      ]
    }
  end

  # support/heaptrack_port.sh rather than `heaptrack`: the stock launcher
  # prints to stdout, which is the port's packet channel.
  defp heaptrack_record(workload, count, output) do
    port = open_port([], [Path.expand("heaptrack_port.sh", __DIR__), output], size(workload))
    run_workload(port, workload, count)

    # A terminated port never flushed its trace, so the data can't be compared.
    if close_and_wait(port) == :terminated do
      raise "the port ignored quit and was terminated; its heaptrack data is incomplete"
    end
  end

  @doc "Delete the files a measurement wrote."
  def delete_files(%{files: files}) do
    for {_label, path} <- files, File.regular?(path), do: File.rm(path)
    :ok
  end

  # ---------------------------------------------------------------------------
  # Printing

  def print_measurement(measurement, limit \\ 12)

  def print_measurement(%{tool: "heaptrack"} = m, limit) do
    IO.puts("  growth in unfreed bytes over #{m.iterations} frames, by allocation stack:")
    IO.puts("         bytes  per_frame  allocated by -> innermost driver frame")

    {shown, rest} = Enum.split(m.rows, limit)

    Enum.each(shown, fn s ->
      per_frame = Float.round(s.bytes / m.iterations, 1)

      IO.puts(
        "    #{pad(s.bytes, 10)}  #{pad(per_frame, 9)}  " <>
          "#{s.alloc} -> #{s.driver_frame}#{label(s)}"
      )
    end)

    if m.rows == [], do: IO.puts("    (none)")
    if rest != [], do: IO.puts("    ... and #{length(rest)} smaller rows")
    print_files(m)
  end

  def print_measurement(m, _limit) do
    if m.summary, do: IO.puts("  #{m.summary}")
    IO.puts("  leaked blocks grouped by allocation stack:")
    IO.puts("    blocks  per_frame       bytes  allocated by -> innermost driver frame")

    Enum.each(m.rows, fn s ->
      per_frame = Float.round(s.count / m.iterations, 1)

      IO.puts(
        "    #{pad(s.count, 6)}  #{pad(per_frame, 9)}  #{pad(s.bytes, 10)}  " <>
          "#{s.alloc} -> #{s.driver_frame}#{label(s)}"
      )
    end)

    print_files(m)
  end

  defp print_files(m) do
    width = m.files |> Enum.map(fn {label, _} -> String.length(label) end) |> Enum.max()

    for {label, path} <- m.files do
      IO.puts("  #{String.pad_trailing(label <> ":", width + 1)} #{path}")
    end
  end

  # The arrays are allocated by cairo_glyph_allocate/cairo_text_cluster_allocate,
  # which the compiler may inline into cairo_scaled_font_text_to_glyphs.
  def label(%{alloc: alloc, driver_frame: frame}) do
    cond do
      alloc =~
        ~r/cairo_scaled_font_text_to_glyphs|cairo_glyph_allocate|cairo_text_cluster_allocate/ and
          frame =~ "script_ops_draw_text" ->
        "  [draw_text glyph/cluster arrays]"

      alloc =~ "_cairo_pattern_create" and frame =~ "script_ops_" ->
        "  [pattern per fill/stroke op, separate leak]"

      gtk_frame?(frame) ->
        "  [GTK main loop, not driver code]"

      true ->
        ""
    end
  end

  # True if the allocation is GTK's, not the driver's: the innermost driver
  # frame is main() (GTK's event loop) or one of the functions that only start
  # GTK up or hand it the finished frame.
  def gtk_frame?(frame), do: frame =~ ~r/^(main|device_init|device_loop|on_draw)\b/

  def pad(v, w), do: v |> to_string() |> String.pad_leading(w)

  # Merge stacks that differ only above the driver (e.g. GTK event paths).
  defp merge_rows(stacks) do
    stacks
    |> Enum.group_by(&{&1.alloc, &1.driver_frame})
    |> Enum.map(fn {_, [first | _] = group} ->
      %{
        first
        | count: sum_counts(group),
          bytes: group |> Enum.map(& &1.bytes) |> Enum.sum()
      }
    end)
    |> Enum.sort_by(&{-(&1.count || 0), -&1.bytes})
  end

  defp sum_counts([%{count: nil} | _]), do: nil
  defp sum_counts(group), do: group |> Enum.map(& &1.count) |> Enum.sum()

  # ---------------------------------------------------------------------------
  # Parsing `leaks` output
  #
  # With MallocStackLogging on, `leaks` merges leaks with an identical stack
  # into one "STACK OF n INSTANCES OF 'ROOT LEAK: ...'" block: the stack
  # (outermost frame first, frame 0 last), "====", then one line per leaked
  # block ending in "[size]".

  def parse_leaks(out) do
    out
    |> String.split(~r/^STACK OF /m)
    |> Enum.drop(1)
    |> Enum.flat_map(fn block ->
      with [_, count] <- Regex.run(~r/^(\d+) INSTANCES? OF 'ROOT LEAK/, block),
           [stack, leaks] <- String.split(block, "\n====\n", parts: 2) do
        frames = stack |> String.split("\n") |> Enum.drop(1) |> Enum.map(&parse_leaks_frame/1)

        [
          %{
            count: String.to_integer(count),
            bytes:
              ~r/ROOT LEAK: .*\[(\d+)\]$/m
              |> Regex.scan(leaks)
              |> Enum.map(fn [_, b] -> String.to_integer(b) end)
              |> Enum.sum(),
            alloc: leaks_alloc_site(frames),
            driver_frame: leaks_driver_frame(frames)
          }
        ]
      else
        _ -> []
      end
    end)
  end

  # "2   scenic_driver_local   0x102e32cc4 script_ops_draw_text + 152  cairo_script_ops.c:274"
  defp parse_leaks_frame(line) do
    case Regex.run(~r/^\s*\d+\s+(\S+)\s+0x[0-9a-f]+\s+(.*?)\s*$/, line) do
      [_, image, symbol] -> {image, symbol}
      _ -> {"?", String.trim(line)}
    end
  end

  # The first frame above malloc/calloc.
  defp leaks_alloc_site(frames) do
    frames
    |> Enum.reverse()
    |> Enum.find_value("?", fn {image, sym} ->
      if image != "libsystem_malloc.dylib", do: sym
    end)
  end

  defp leaks_driver_frame(frames) do
    frames
    |> Enum.reverse()
    |> Enum.find_value("(none)", fn {image, sym} ->
      if image == "scenic_driver_local", do: sym
    end)
  end

  # ---------------------------------------------------------------------------
  # Parsing valgrind memcheck output
  #
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
          driver = Enum.drop_while(frames, &(not valgrind_driver_frame?(&1, sources)))

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
  defp valgrind_driver_frame?({_sym, where}, sources) do
    String.ends_with?(where, "/scenic_driver_local") or
      MapSet.member?(sources, where |> String.split(":") |> hd())
  end

  defp driver_sources do
    Path.expand("../../../c_src/**/*.c", __DIR__)
    |> Path.wildcard()
    |> MapSet.new(&Path.basename/1)
  end

  defp to_int(str), do: str |> String.replace(",", "") |> String.to_integer()

  # ---------------------------------------------------------------------------
  # Parsing heaptrack_print's flame graph file
  #
  # One line per allocation stack, outermost frame first, then the cost (here:
  # growth in bytes never freed):
  #
  #   __clone3;...;render_script (script.c);script_ops_fill_color (cairo_script_ops.c);cairo_pattern_create_rgba;_cairo_pattern_create_solid; 192024
  #
  # Frames with debug info carry their source file, which is how driver frames
  # are recognised. Stacks that didn't grow or have no driver frame are dropped.

  def parse_heaptrack_stacks(text) do
    sources = driver_sources()
    lines = String.split(text, "\n", trim: true)

    # Every stack passes through main(), so if no line has a driver frame the
    # binary has no debug info and every row would be silently dropped.
    if lines != [] and not Enum.any?(lines, &heaptrack_has_driver_frame?(&1, sources)) do
      raise "no driver frames in the heaptrack stacks: build the port with debug info (-g)"
    end

    lines
    |> Enum.flat_map(fn line ->
      with [_, stack, cost] <- Regex.run(~r/^(.*) (-?\d+)$/, line),
           frames = stack |> String.split(";", trim: true) |> Enum.reverse(),
           {inner, [driver | _]} <-
             Enum.split_while(frames, &(not heaptrack_driver_frame?(&1, sources))) do
        [
          %{
            count: nil,
            bytes: String.to_integer(cost),
            alloc: strip_source(List.first(inner, driver)),
            driver_frame: strip_source(driver)
          }
        ]
      else
        _ -> []
      end
    end)
    |> merge_rows()
    |> Enum.filter(&(&1.bytes > 0))
  end

  defp heaptrack_has_driver_frame?(line, sources) do
    line |> String.split(";") |> Enum.any?(&heaptrack_driver_frame?(&1, sources))
  end

  defp heaptrack_driver_frame?(frame, sources) do
    case Regex.run(~r/ \(([^()]+)\)$/, frame) do
      [_, file] -> MapSet.member?(sources, Path.basename(file))
      _ -> false
    end
  end

  defp strip_source(frame), do: String.replace(frame, ~r/ \([^()]+\)$/, "")
end
