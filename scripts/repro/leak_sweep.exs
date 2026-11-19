# Sweep the driver's script ops and port commands for leaks.
#
# Each case exercises one op or command in isolation, in its own port process:
# a small "_root_" script containing the op, rendered once per iteration, or a
# port command sent once per iteration and followed by a render. The case is
# measured with a heap tool (see support/harness.exs) and reported as memory
# leaked per iteration, grouped by allocation site.
#
# Draw ops need a fill and a stroke to draw with. The cases set both to an
# image, not a colour, because the colour and gradient ops are themselves
# cases; that keeps every case's result about its own op.
#
# Run through scripts/repro/leak_sweep.sh, which builds the cairo-gtk binary
# first. Settings come from environment variables:
#
#   CASES        only run cases whose name or group contains one of these
#                comma separated strings, e.g. "paint,draw_text"
#   ITERATIONS   measured iterations per case          (default 200)
#   WARMUP       iterations before measuring starts    (default 50)
#   MIN_BYTES    heaptrack only: growth below this many bytes per iteration
#                at one allocation site is ignored      (default 16)
#   LEAK_TOOL    auto | leaks | heaptrack | valgrind   (default auto: the first
#                of leaks, heaptrack, valgrind that is installed)
#   LIST=1       print the case names and exit

Code.require_file("support/harness.exs", __DIR__)

defmodule LeakSweep do
  alias Scenic.Driver.Local.ToPort
  alias Scenic.Script

  @font "font"
  @image "img"
  @image_size 16

  # ---------------------------------------------------------------------------
  # Cases

  def cases do
    script_cases() ++ command_cases()
  end

  # {name, group, paint?, fn ops -> ops}. With paint?, the script starts by
  # setting the fill and stroke to an image so the op has something to draw
  # with.
  defp script_cases do
    [
      {"empty_script", "baseline", false, & &1},
      # state and transforms
      {"push_pop_state", "state", false, &(&1 |> Script.push_state() |> Script.pop_state())},
      {"pop_push_state", "state", false,
       &(&1 |> Script.push_state() |> Script.pop_push_state() |> Script.pop_state())},
      {"push_state_unbalanced", "state", false, &Script.push_state/1},
      {"pop_state_unbalanced", "state", false, &Script.pop_state/1},
      {"scissor", "state", true,
       &(&1 |> Script.scissor(50, 50) |> Script.draw_rectangle(80, 80, :fill))},
      {"transform", "state", false, &Script.transform(&1, 1, 0, 0, 1, 10, 10)},
      {"scale", "state", false, &Script.scale(&1, 1.5, 1.5)},
      {"rotate", "state", false, &Script.rotate(&1, 0.3)},
      {"translate", "state", false, &Script.translate(&1, 10, 10)},
      # paint
      {"fill_color", "paint", false, &Script.fill_color(&1, :red)},
      {"fill_linear", "paint", false, &Script.fill_linear(&1, 0, 0, 100, 100, :red, :blue)},
      {"fill_radial", "paint", false, &Script.fill_radial(&1, 50, 50, 5, 40, :red, :blue)},
      {"fill_image", "paint", false, &[{:fill_image, @image} | &1]},
      {"fill_image_missing", "paint", false, &[{:fill_image, "nope"} | &1]},
      {"fill_stream", "paint", false, &Script.fill_stream(&1, @image)},
      {"stroke_color", "paint", false, &Script.stroke_color(&1, :green)},
      {"stroke_linear", "paint", false, &Script.stroke_linear(&1, 0, 0, 100, 100, :red, :blue)},
      {"stroke_radial", "paint", false, &Script.stroke_radial(&1, 50, 50, 5, 40, :red, :blue)},
      {"stroke_image", "paint", false, &[{:stroke_image, @image} | &1]},
      {"stroke_stream", "paint", false, &Script.stroke_stream(&1, @image)},
      {"stroke_width", "paint", false, &Script.stroke_width(&1, 3)},
      {"cap", "paint", false, &Script.cap(&1, :round)},
      {"join", "paint", false, &Script.join(&1, :round)},
      {"miter_limit", "paint", false, &Script.miter_limit(&1, 4)},
      # colours as they are really used: set, then drawn with
      {"fill_color_drawn", "paint", false,
       &(&1 |> Script.fill_color(:red) |> Script.draw_rectangle(60, 40, :fill))},
      {"stroke_color_drawn", "paint", false,
       &(&1 |> Script.stroke_color(:green) |> Script.draw_rectangle(60, 40, :stroke))},
      {"fill_color_in_state", "paint", false,
       &(&1
         |> Script.push_state()
         |> Script.fill_color(:red)
         |> Script.draw_rectangle(60, 40, :fill)
         |> Script.pop_state())},
      # draw primitives
      {"draw_line", "draw", true, &Script.draw_line(&1, 0, 0, 100, 100, :stroke)},
      {"draw_triangle", "draw", true,
       &Script.draw_triangle(&1, 10, 10, 90, 10, 50, 80, :fill_stroke)},
      {"draw_quad", "draw", true,
       &Script.draw_quad(&1, 10, 10, 90, 20, 80, 90, 20, 80, :fill_stroke)},
      {"draw_rect", "draw", true, &Script.draw_rectangle(&1, 80, 60, :fill_stroke)},
      {"draw_rrect", "draw", true, &Script.draw_rounded_rectangle(&1, 80, 60, 8, :fill_stroke)},
      {"draw_rrectv", "draw", true,
       &Script.draw_variable_rounded_rectangle(&1, 80, 60, 2, 4, 8, 16, :fill_stroke)},
      {"draw_arc", "draw", true, &Script.draw_arc(&1, 40, 1.5, :fill_stroke)},
      {"draw_sector", "draw", true, &Script.draw_sector(&1, 40, 1.5, :fill_stroke)},
      {"draw_circle", "draw", true, &Script.draw_circle(&1, 40, :fill_stroke)},
      {"draw_ellipse", "draw", true, &Script.draw_ellipse(&1, 40, 20, :fill_stroke)},
      {"draw_sprites", "draw", false,
       &[{:draw_sprites, {@image, [{{0, 0}, {16, 16}, {10, 10}, {32, 32}, 1.0}]}} | &1]},
      {"draw_sprites_missing", "draw", false,
       &[{:draw_sprites, {"nope", [{{0, 0}, {16, 16}, {10, 10}, {32, 32}, 1.0}]}} | &1]},
      {"draw_script", "draw", true, &Script.render_script(&1, "sub")},
      {"draw_script_missing", "draw", false, &Script.render_script(&1, "nope")},
      # paths
      {"path_lines", "path", true,
       &(&1
         |> Script.begin_path()
         |> Script.move_to(10, 10)
         |> Script.line_to(90, 10)
         |> Script.line_to(50, 80)
         |> Script.close_path()
         |> Script.fill_path()
         |> Script.stroke_path())},
      {"path_arc_to", "path", true,
       &(&1
         |> Script.begin_path()
         |> Script.move_to(10, 10)
         |> Script.arc_to(90, 10, 90, 90, 20)
         |> Script.stroke_path())},
      {"path_bezier_to", "path", true,
       &(&1
         |> Script.begin_path()
         |> Script.move_to(10, 10)
         |> Script.bezier_to(30, 0, 70, 100, 90, 50)
         |> Script.stroke_path())},
      {"path_quadratic_to", "path", true,
       &(&1
         |> Script.begin_path()
         |> Script.move_to(10, 10)
         |> Script.quadratic_to(50, 100, 90, 10)
         |> Script.stroke_path())},
      {"path_arc", "path", true,
       &(&1
         |> Script.begin_path()
         |> Script.arc(50, 50, 30, 0, 2, 1)
         |> Script.stroke_path())},
      {"path_unfinished", "path", true,
       &(&1 |> Script.begin_path() |> Script.move_to(10, 10) |> Script.line_to(90, 10))},
      # text
      {"font", "text", false, &[{:font, @font} | &1]},
      {"font_missing", "text", false, &[{:font, "nope"} | &1]},
      {"font_size", "text", false, &Script.font_size(&1, 24)},
      {"text_align", "text", false, &Script.text_align(&1, :center)},
      {"text_base", "text", false, &Script.text_base(&1, :middle)},
      {"draw_text", "text", true, &(&1 |> text_style() |> Script.draw_text("Hello, world"))},
      {"draw_text_aligned", "text", true,
       &(&1
         |> text_style()
         |> Script.text_align(:right)
         |> Script.text_base(:top)
         |> Script.draw_text("Hello, world"))},
      {"draw_text_empty", "text", true, &(&1 |> text_style() |> Script.draw_text(""))},
      {"draw_text_no_font", "text", true, &Script.draw_text(&1, "Hello, world")},
      {"draw_text_unicode", "text", true,
       &(&1 |> text_style() |> Script.draw_text("héllo wörld ✓ 日本"))}
    ]
    |> Enum.map(fn {name, group, paint?, build} ->
      root = Script.start() |> then(&if(paint?, do: paint(&1), else: &1)) |> build.()

      %{
        name: name,
        group: group,
        note: nil,
        setup: fn port ->
          common_setup(port)
          ToPort.put_script(Repro.serialize(root), "_root_", port)
        end,
        step: fn _port, _i -> :ok end
      }
    end)
  end

  # Port commands, sent once per iteration. A frame is rendered after each.
  defp command_cases do
    rgba = pixels(4)

    [
      command("put_script_replace", fn port, i ->
        ToPort.put_script(rect_script(10 + rem(i, 7)), "_root_", port)
      end),
      command("put_script_then_delete", fn port, i ->
        ToPort.put_script(rect_script(20), "tmp_#{i}", port)
        ToPort.del_script("tmp_#{i}", port)
      end),
      command("del_script_missing", fn port, _i -> ToPort.del_script("nope", port) end),
      command("reset", fn port, _i ->
        ToPort.reset_start(port)
        ToPort.put_script(rect_script(20), "_root_", port)
      end),
      command("clear_color", fn port, i ->
        ToPort.clear_color({rem(i, 255), 0, 0, 255}, port)
      end),
      command("global_tx", fn port, _i -> ToPort.set_global_tx(matrix(), port) end),
      command("cursor_tx", fn port, _i -> ToPort.set_cursor_tx(matrix(), port) end),
      command(
        "update_cursor",
        fn port, i -> ToPort.update_cursor(true, {rem(i, 100), 10}, port) end,
        setup: fn port -> ToPort.put_script(rect_script(8), "_cursor_", port) end
      ),
      command("request_inputs", fn port, _i ->
        ToPort.request_inputs([:cursor_pos, :key], port)
      end),
      command("window_commands", fn port, _i ->
        ToPort.reshape(400, 300, port)
        ToPort.position(10, 10, port)
        ToPort.focus(port)
      end),
      command("put_font_same_id", fn port, _i -> ToPort.put_font(port, @font, font()) end),
      command("put_font_new_id", fn port, i -> ToPort.put_font(port, "font_#{i}", font()) end,
        note: "no command deletes a font"
      ),
      command("put_font_corrupt", fn port, i ->
        ToPort.put_font(port, "bad_#{i}", "this is not a font file")
      end),
      command("put_texture_rgba", fn port, _i ->
        ToPort.put_texture(port, @image, :rgba, @image_size, @image_size, rgba)
      end),
      command("put_texture_rgb", fn port, _i ->
        ToPort.put_texture(port, @image, :rgb, @image_size, @image_size, pixels(3))
      end),
      command("put_texture_g", fn port, _i ->
        ToPort.put_texture(port, @image, :g, @image_size, @image_size, pixels(1))
      end),
      command("put_texture_ga", fn port, _i ->
        ToPort.put_texture(port, @image, :ga, @image_size, @image_size, pixels(2))
      end),
      command("put_texture_file", fn port, _i ->
        ToPort.put_texture(port, @image, :file, @image_size, @image_size, png(@image_size))
      end),
      command(
        "put_texture_new_id",
        fn port, i ->
          ToPort.put_texture(port, "img_#{i}", :rgba, @image_size, @image_size, rgba)
        end,
        note: "no command deletes an image"
      ),
      command("put_texture_resized", fn port, _i ->
        ToPort.put_texture(port, @image, :rgba, 8, 8, binary_part(rgba, 0, 8 * 8 * 4))
      end),
      command("put_texture_file_wrong_size", fn port, _i ->
        ToPort.put_texture(port, @image, :file, @image_size, @image_size, png(8))
      end),
      command("put_texture_file_corrupt", fn port, _i ->
        ToPort.put_texture(port, @image, :file, @image_size, @image_size, "not an image")
      end)
    ]
  end

  defp command(name, step, opts \\ []) do
    extra_setup = Keyword.get(opts, :setup, fn _port -> :ok end)

    %{
      name: name,
      group: "command",
      note: opts[:note],
      setup: fn port ->
        common_setup(port)
        ToPort.put_script(rect_script(20), "_root_", port)
        extra_setup.(port)
      end,
      step: step
    }
  end

  # ---------------------------------------------------------------------------
  # Shared fixtures

  defp common_setup(port) do
    ToPort.put_font(port, @font, font())
    ToPort.put_texture(port, @image, :rgba, @image_size, @image_size, pixels(4))
    ToPort.clear_color(:black, port)
    ToPort.put_script(rect_script(12), "sub", port)
  end

  # Fill and stroke with an image: unlike the colour ops this allocates
  # nothing per frame, so it doesn't show up in other cases' results.
  defp paint(ops) do
    [{:stroke_image, @image}, {:fill_image, @image} | ops] |> Script.stroke_width(2)
  end

  defp text_style(ops), do: [{:font, @font} | ops] |> Script.font_size(18)

  defp rect_script(size) do
    Script.start()
    |> paint()
    |> Script.draw_rectangle(size, size, :fill_stroke)
    |> Repro.serialize()
  end

  defp font do
    case :persistent_term.get({__MODULE__, :font}, nil) do
      nil ->
        bin = Repro.roboto()
        :persistent_term.put({__MODULE__, :font}, bin)
        bin

      bin ->
        bin
    end
  end

  # A 4x4 matrix, as Scenic.Math.Matrix lays it out: identity plus a small translation.
  defp matrix do
    for v <- [1, 0, 0, 2, 0, 1, 0, 2, 0, 0, 1, 0, 0, 0, 0, 1],
        into: <<>>,
        do: <<v::float-32-native>>
  end

  defp pixels(channels) do
    for i <- 1..(@image_size * @image_size * channels), into: <<>>, do: <<rem(i * 37, 256)>>
  end

  # A minimal RGBA PNG of size x size.
  defp png(size) do
    rows =
      for _ <- 1..size, into: <<>>, do: <<0, :binary.copy(<<200, 80, 40, 255>>, size)::binary>>

    chunk = fn type, data ->
      <<byte_size(data)::32, type::binary, data::binary, :erlang.crc32(type <> data)::32>>
    end

    <<137, "PNG", 13, 10, 26, 10>> <>
      chunk.("IHDR", <<size::32, size::32, 8, 6, 0, 0, 0>>) <>
      chunk.("IDAT", :zlib.compress(rows)) <>
      chunk.("IEND", <<>>)
  end

  # ---------------------------------------------------------------------------
  # Running

  def selected_cases do
    case System.get_env("CASES") do
      nil ->
        cases()

      filter ->
        wanted = String.split(filter, ",", trim: true)

        Enum.filter(cases(), fn c ->
          Enum.any?(wanted, &(String.contains?(c.name, &1) or c.group == &1))
        end)
    end
  end

  def run(tool) do
    cases = selected_cases()
    n = Repro.env_int("ITERATIONS", 200)
    warmup = Repro.env_int("WARMUP", 50)
    dir = Path.join(System.tmp_dir!(), "scenic_leak_sweep_#{System.pid()}")
    File.mkdir_p!(dir)

    IO.puts("")

    IO.puts(
      "leak sweep: #{length(cases)} cases, #{tool}, #{warmup} warm-up + #{n} iterations each"
    )

    IO.puts("  #{String.pad_trailing("case", 30)} bytes/iter  result")

    results =
      Enum.map(cases, fn c ->
        result = run_case(c, tool, warmup, n, Path.join(dir, c.name))
        IO.puts("  #{String.pad_trailing(c.name, 30)} #{result_line(result)}")
        result
      end)

    print_sites(results)
    print_gtk(results)
    print_problems(results)
    File.rmdir(dir)
  end

  defp run_case(c, tool, warmup, n, base) do
    m = Repro.measure(tool, c, warmup, n, base)
    {gtk, driver} = Enum.split_with(m.rows, &Repro.gtk_frame?(&1.driver_frame))
    leaks = Enum.filter(driver, &(per_iter(&1, m) >= threshold(tool)))

    if leaks == [], do: Repro.delete_files(m)

    %{
      case: c,
      status:
        cond do
          leaks == [] -> :ok
          c.note -> :by_design
          true -> :leak
        end,
      leaks: Enum.map(leaks, &Map.put(&1, :per_iter, per_iter(&1, m))),
      gtk: Enum.map(gtk, &Map.put(&1, :per_iter, per_iter(&1, m))),
      bytes_per_iter: leaks |> Enum.map(& &1.bytes) |> Enum.sum() |> Kernel./(m.iterations),
      files: if(leaks == [], do: [], else: m.files)
    }
  rescue
    e ->
      # The port died or hung; stop it and drain what it left in the mailbox.
      Repro.kill_all()
      flush()
      %{case: c, status: :error, error: Exception.message(e), leaks: [], gtk: [], files: []}
  end

  defp flush do
    receive do
      _ -> flush()
    after
      0 -> :ok
    end
  end

  # leaks and valgrind count blocks, heaptrack only bytes.
  defp per_iter(%{count: nil, bytes: bytes}, m), do: bytes / m.iterations
  defp per_iter(%{count: count}, m), do: count / m.iterations

  # heaptrack compares two runs, and one-off allocations in GTK and Xlib don't
  # always land in both, which shows up as a few hundred bytes of "growth"
  # over the whole run. A real per-iteration leak is at least one block.
  defp threshold("heaptrack"), do: Repro.env_int("MIN_BYTES", 16)
  defp threshold(_blocks), do: 0.5

  defp result_line(%{status: :ok}), do: "#{Repro.pad("0", 10)}  ok"

  defp result_line(%{status: :error, error: error}) do
    "#{Repro.pad("-", 10)}  FAILED: #{error}"
  end

  defp result_line(%{status: :by_design} = r) do
    "#{Repro.pad(Float.round(r.bytes_per_iter, 1), 10)}  grows by design: #{r.case.note}"
  end

  defp result_line(%{status: :leak} = r) do
    sites = r.leaks |> Enum.map(&"#{&1.alloc} -> #{&1.driver_frame}") |> Enum.join(", ")
    "#{Repro.pad(Float.round(r.bytes_per_iter, 1), 10)}  LEAK: #{sites}"
  end

  # Every leaking allocation site, with the cases it showed up in.
  defp print_sites(results) do
    sites =
      for r <- results, r.status == :leak, row <- r.leaks do
        {{row.alloc, row.driver_frame}, {r.case.name, row.bytes / row_iterations(row)}}
      end
      |> Enum.group_by(&elem(&1, 0), &elem(&1, 1))
      |> Enum.sort_by(fn {_site, seen} -> -(seen |> Enum.map(&elem(&1, 1)) |> Enum.max()) end)

    IO.puts("")

    if sites == [] do
      IO.puts("no leaks in driver code")
    else
      IO.puts("leak sites in driver code (allocated by -> innermost driver frame):")

      for {{alloc, frame}, seen} <- sites do
        {_, most} = Enum.max_by(seen, &elem(&1, 1))
        names = Enum.map(seen, &elem(&1, 0))
        IO.puts("  #{alloc} -> #{frame}")

        IO.puts(
          "      up to #{Float.round(most, 1)} bytes/iter in #{length(names)} case(s): #{shorten(names)}"
        )
      end
    end
  end

  defp row_iterations(%{per_iter: per_iter, count: nil, bytes: bytes}), do: bytes / per_iter
  defp row_iterations(%{per_iter: per_iter, count: count}), do: count / per_iter

  defp shorten(names) do
    {shown, rest} = Enum.split(names, 8)
    Enum.join(shown, ", ") <> if(rest == [], do: "", else: ", ... (#{length(rest)} more)")
  end

  # Allocations whose only driver frame is main() belong to GTK's own event
  # loop and drawing. They are listed once, averaged over the cases, so a
  # GTK-side problem is still visible without being blamed on a case.
  defp print_gtk(results) do
    measured = Enum.count(results, &(&1.status != :error))

    rows =
      for r <- results, row <- r.gtk do
        {row.alloc, row.bytes / row_iterations(row)}
      end
      |> Enum.group_by(&elem(&1, 0), &elem(&1, 1))
      |> Enum.map(fn {alloc, per_iters} -> {alloc, Enum.sum(per_iters) / max(measured, 1)} end)
      |> Enum.filter(fn {_alloc, avg} -> avg >= 16 end)
      |> Enum.sort_by(&(-elem(&1, 1)))

    if rows != [] do
      IO.puts("")
      IO.puts("growth in GTK's main loop, not driver code (bytes/iter, averaged over all cases):")
      for {alloc, avg} <- rows, do: IO.puts("  #{Repro.pad(Float.round(avg, 1), 8)}  #{alloc}")
    end
  end

  defp print_problems(results) do
    failed = Enum.filter(results, &(&1.status == :error))
    leaking = Enum.filter(results, &(&1.status == :leak))

    by_design = Enum.filter(results, &(&1.status == :by_design))

    if failed != [] do
      IO.puts("")
      IO.puts("cases that could not be measured (the port crashed or hung):")
      for r <- failed, do: IO.puts("  #{r.case.name}: #{r.error}")
    end

    case leaking ++ by_design do
      [] ->
        :ok

      [first | _] ->
        IO.puts("")
        IO.puts("tool output for these cases is under:")
        IO.puts("  #{first.files |> hd() |> elem(1) |> Path.dirname()}")
    end

    if File.exists?(Repro.stderr_log()) do
      IO.puts("")
      IO.puts("the ports' stderr (GTK and tool messages): #{Repro.stderr_log()}")
    end

    IO.puts("")

    IO.puts(
      "#{length(results)} cases: #{Enum.count(results, &(&1.status == :ok))} ok, " <>
        "#{length(leaking)} leaking, #{length(by_design)} growing by design, " <>
        "#{length(failed)} failed"
    )
  end
end

Process.flag(:trap_exit, true)

if System.get_env("LIST") == "1" do
  for c <- LeakSweep.cases(), do: IO.puts("#{String.pad_trailing(c.group, 10)} #{c.name}")
else
  for tool <- Repro.leak_tools(~w(leaks heaptrack valgrind)), do: LeakSweep.run(tool)
end
