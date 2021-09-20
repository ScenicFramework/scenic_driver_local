#
#  Created by Boyd Multerer on 2021-09-04.
#  Copyright 2018-2021 Kry10 Limited
#

defmodule Scenic.Driver.Local do
  @default_limit 29
  @default_layer 0
  @default_opacity 255

  @position_schema [
    scaled: [type: :boolean, default: false],
    centered: [type: :boolean, default: false],
    orientation: [type: {:in, [:normal, :left, :right, :upside_down]}, default: :normal]
  ]

  @window_schema [
    title: [type: :string, default: "Scenic Window"],
    resizeable: [type: :boolean, default: false]
  ]

  @opts_schema [
    name: [type: {:or, [:atom, :string]}],
    limit_ms: [type: :non_neg_integer, default: @default_limit],
    layer: [type: :integer, default: @default_layer],
    opacity: [type: :integer, default: @default_opacity],
    debug: [type: :boolean, default: false],
    calibration: [
      type: {:custom, __MODULE__, :validate_calibration, []},
      default: []
    ],
    position: [type: :keyword_list, keys: @position_schema, default: []],
    window: [type: :keyword_list, keys: @window_schema, default: []],
    # cursor: [type: {:in, [:none, :pointer, :crosshair, :text, :hand]}, default: :none],
    cursor: [type: :boolean, default: false],
    key_mapper: [type: :atom, default: Scenic.KeyMap.USEnglish],
    on_close: [
      type:
        {:or, [:mfa, {:in, [:restart, :stop_driver, :stop_viewport, :stop_system, :halt_system]}]},
      default: :restart
    ],
    antialias: [type: :boolean, default: true]
  ]

  # figure out what device we are targeting. 
  @mix_target (case function_exported?(Mix.Nerves.Utils, :mix_target, 0) do
      true -> Mix.Nerves.Utils.mix_target()
      false -> Mix.target()
    end)


  @moduledoc """
  Documentation for `Scenic.Driver.Local`.

  Supported config options:\n#{NimbleOptions.docs(@opts_schema)}

  """

  use Scenic.Driver
  require Logger

  import IEx

  alias Scenic.Driver

  alias Scenic.Driver.Local.Calbacks
  alias Scenic.Driver.Local.Input
  alias Scenic.Driver.Local.ToPort
  alias Scenic.Driver.Local.FromPort
  alias Scenic.Driver.Local.Cursor

  alias Scenic.Math.Matrix
  alias Scenic.Math.Vector2

  @port '/scenic_driver_local'

  # @root_id Scenic.ViewPort.root_id()

  # pid = Process.whereis( :local )
  # pid = pid(0, 1310, 0)
  # GenServer.call( pid, :dump )

  # Process.alive?(pid)

  # Scenic.Driver.Local.position( pid, scaled: false )
  # Scenic.Driver.Local.position( pid, centered: false )
  # Scenic.Driver.Local.position( pid, orientation: :normal )
  # Scenic.Driver.Local.position( pid, orientation: :left )

  # Scenic.Driver.Local.cursor( pid, :crosshair )

  # ============================================================================
  # external api

  @impl Scenic.Driver
  def validate_opts(opts), do: NimbleOptions.validate(opts, @opts_schema)

  def validate_calibration(calibrations) do
    Enum.all?(calibrations, fn {name, {{ax, bx, dx}, {ay, by, dy}}} ->
      is_bitstring(name) &&
        is_number(ax) &&
        is_number(bx) &&
        is_number(dx) &&
        is_number(ay) &&
        is_number(by) &&
        is_number(dy)
    end)
    |> case do
      true ->
        {:ok, calibrations}

      false ->
        {
          :error,
          """
          #{IO.ANSI.red()}#{__MODULE__}: Invalid calibration option.
          This must be a list of devices names and calibration data.
          Calibration data must be in the format of {{ax, bx, dx},{ay, by, dy}}
          where all values are numbers.

          You don't need to set this for devices that have a 1:1 mapping, such
          as the standard pi touchscreens. But if you did it would look like this...

          [{ "FT5406 memory based driver", {{1,0,0},{0,1,0}} }]
          #{IO.ANSI.yellow()}Received: #{inspect(calibrations)}
          #{IO.ANSI.default_color()}
          """
        }
    end
  end

  @spec position(driver :: pid | Driver.t(), otps :: Keyword.t()) ::
          :ok | NimbleOptions.ValidationError.t()
  def position(%Scenic.Driver{pid: pid}, opts), do: position(pid, opts)

  def position(pid, opts) when is_list(opts) do
    case NimbleOptions.validate(opts, @position_schema) do
      {:ok, _} ->
        opts =
          []
          |> put_if_set(:scaled, opts[:scaled])
          |> put_if_set(:centered, opts[:centered])
          |> put_if_set(:orientation, opts[:orientation])

        send(pid, {:_position_, opts})

      err ->
        err
    end
  end

  # not ready to expose these yet. I want to think through the cursor model better
  # it will probably end up as a :cursor style that you can place on items. Then the 
  # cursor type would change as it moves over those items.

  # the reason I'm not exposing this now is that it would require the :cursor_pos
  # input type to be enabled all the time. This would be really bad for remote
  # scenarios. Instead, I want the remote client to be able to change the cursor
  # locally without going back to the device. This is especially important for
  # read-only remote clients, which should be able to scale up to an arbitrary
  # number of clients.

  @spec set_cursor(driver :: pid | Driver.t(), cursor :: Cursor.t()) :: :ok
  def set_cursor(%Scenic.Driver{pid: pid}, cursor), do: position(pid, cursor)

  def set_cursor(pid, cursor) when is_atom(cursor) or is_list(cursor) do
    Process.send(pid, {:_set_cursor_, cursor}, [])
  end

  @spec show_cursor(driver :: pid | Driver.t()) :: :ok
  def show_cursor(%Scenic.Driver{pid: pid}), do: show_cursor(pid)

  def show_cursor(pid) do
    Process.send(pid, :_show_cursor_, [])
  end

  @spec hide_cursor(driver :: pid | Driver.t()) :: :ok
  def hide_cursor(%Scenic.Driver{pid: pid}), do: show_cursor(pid)

  def hide_cursor(pid) do
    Process.send(pid, :_hide_cursor_, [])
  end

  defp put_if_set(opts, key, value)
  defp put_if_set(opts, _key, nil), do: opts

  defp put_if_set(opts, key, value) do
    Keyword.put(opts, key, value)
  end

  # ============================================================================
  # startup
  @doc false
  @impl Scenic.Driver
  def init(driver, opts) do
    {width, height} = driver.viewport.size

    Logger.info("#{inspect(__MODULE__)}: start: #{inspect(opts)}, pid: #{inspect(self())}")

    # set up the port args - enforce type checking
    debug_mode =
      case opts[:debug] do
        true -> 1
        false -> 0
      end

    internal_cursor =
      case opts[:cursor] do
        true -> 1
        false -> 0
      end

    antialias =
      case opts[:antialias] do
        true -> 1
        false -> 0
      end

    {:ok, layer} = Keyword.fetch(opts, :layer)
    {:ok, opacity} = Keyword.fetch(opts, :opacity)

    {:ok, window_opts} = Keyword.fetch(opts, :window)
    {:ok, title} = Keyword.fetch(window_opts, :title)

    resizeable =
      case window_opts[:resizeable] do
        true -> 1
        false -> 0
      end

    args =
      " #{internal_cursor} #{layer} #{opacity} #{antialias} #{debug_mode}" <>
        " #{width} #{height} #{resizeable} \"#{title}\""

    IO.inspect(args, label: "args")

    # open and initialize the window
    Process.flag(:trap_exit, true)

    executable =
      (:code.priv_dir(:scenic_driver_local) ++ @port ++ to_charlist(args))
      |> IO.inspect(label: "executable")

    port = Port.open({:spawn, executable}, [:binary, {:packet, 4}])

    driver =
      assign(driver,
        port: port,
        closing: false,
        screen_factor: 1.0,
        logical_size: {width, height},
        window_size: {width, height},
        on_close: opts[:on_close],
        media: %{},
        position: opts[:position],
        busy: true,
        calibration: opts[:calibration],
        input_state: %{},
        input_debounce: [],
        key_mapper: opts[:key_mapper],
        keys: %{},
        mapper_state: nil,
        tx: Scenic.Math.Matrix.identity(),
        inv_tx: Scenic.Math.Matrix.identity(),
        rel_tx: Scenic.Math.Matrix.identity(),
        inv_rel_tx: Scenic.Math.Matrix.identity(),
        auto_cursor: opts[:cursor],
        cursor_showing: false,
        cursor_pos: {0, 0},
        last_cursor: nil,
        cursor_update: false,
        rel_x: 0,
        rel_y: 0
      )

    # send message to set up the cursor later
    send(self(), {:_set_cursor_, :touch_spot})

    # send message so input handling gets set up later
    if @mix_target != :host, do: send(self(), :_init_input_)

    {:ok, driver}
  end

  # --------------------------------------------------------
  # Delegate the callbacks

  # @doc false
  # @impl Scenic.Driver
  # defdelegate request_input(input, driver), to: Input

  @doc false
  @impl Scenic.Driver
  defdelegate reset_scene(driver), to: Calbacks

  @doc false
  @impl Scenic.Driver
  defdelegate update_scene(ids, driver), to: Calbacks

  @doc false
  @impl Scenic.Driver
  defdelegate del_scripts(ids, driver), to: Calbacks

  @doc false
  @impl Scenic.Driver
  defdelegate clear_color(color, driver), to: Calbacks

  # --------------------------------------------------------

  @doc false
  @impl GenServer
  def handle_call(:dump, _, driver) do
    {:reply, driver, driver}
  end

  # --------------------------------------------------------

  @doc false
  @impl GenServer

  # messages from the port
  def handle_info({pid, {:data, data}}, %{assigns: %{port: port}} = driver) when pid == port do
    FromPort.handle_port_message(data, driver)
  end

  # messages from input
  def handle_info({:input_event, source, events}, driver) do
    # Logger.warn "input_event - #{inspect(source)}: #{inspect(events)}"
    Input.handle_input(source, events, driver)
  end

  # handle asset stream updates
  def handle_info({{Scenic.Assets.Stream, _}, type, id}, driver) do
    Calbacks.handle_put_stream(type, id, driver)
  end

  # deal with the positioning api
  def handle_info(
        {:_position_, new_opts},
        %{assigns: %{position: old_opts}} = driver
      ) do
    # merge opts allowing partial updates
    new_opts =
      new_opts
      |> Keyword.put_new(:scaled, Keyword.get(old_opts, :scaled))
      |> Keyword.put_new(:centered, Keyword.get(old_opts, :centered))
      |> Keyword.put_new(:orientation, Keyword.get(old_opts, :orientation))

    driver =
      driver
      |> assign(:position, new_opts)
      |> set_global_tx()
      |> Driver.request_update()

    {:noreply, driver}
  end

  # handle the port exiting
  def handle_info(
        {:EXIT, port_id, :normal},
        %{assigns: %{port: port, closing: closing}} = driver
      )
      when port_id == port do
    if closing do
      Logger.info("Scenic RPI Driver clean close")
      # we are closing cleanly, let it happen.
      GenServer.stop(self())
      {:noreply, driver}
    else
      Logger.error("Scenic RPI Driver dirty close")
      # we are not closing cleanly. Let the supervisor recover.
      {:noreply, driver}
    end
  end

  def handle_info(:_init_input_, driver) do
    {:noreply, Input.init_input(driver)}
  end

  def handle_info({:_set_cursor_, cursor}, driver) do
    {:noreply, Cursor.set(driver, cursor)}
  end

  def handle_info(:_hide_cursor_, driver) do
    {:noreply, Cursor.hide(driver)}
  end

  def handle_info(:_show_cursor_, driver) do
    {:noreply, Cursor.hide(driver)}
  end

  def handle_info({:_clear_input_debounce_, source, event}, driver) do
    {:noreply, Input.clear_input_debounce(source, event, driver)}
  end

  def handle_info(_msg, driver) do
    # Logger.warn("#{inspect(__MODULE__)} ignoring #{inspect(msg)}")
    {:noreply, driver}
  end

  # --------------------------------------------------------
  # internal helper utilities

  @doc false
  # not defp because it is called from input.ex
  def set_global_tx(
        %{
          assigns: %{
            window_size: {win_w, win_h},
            logical_size: {log_w, log_h},
            position: opts,
            port: port
          }
        } = driver
      ) do
    pin = {log_w / 2, log_h / 2}
    inv_pin = Vector2.invert(pin)

    {centered, center_trans} =
      case opts[:centered] do
        false -> {false, {0, 0}}
        true -> {true, {win_w / 2 - log_w / 2, win_h / 2 - log_h / 2}}
      end

    scale =
      case opts[:scaled] do
        true ->
          case opts[:orientation] do
            :left ->
              case abs(win_w / log_w) > abs(win_h / log_h) do
                true -> win_h / log_w
                false -> win_w / log_h
              end

            :right ->
              case abs(win_w / log_w) > abs(win_h / log_h) do
                true -> win_h / log_w
                false -> win_w / log_h
              end

            _ ->
              case abs(win_w / log_w) < abs(win_h / log_h) do
                true -> win_w / log_w
                false -> win_h / log_h
              end
          end

        _ ->
          1.0
      end

    {rotated, rot_angle, rot_trans} =
      case opts[:orientation] do
        :normal -> {false, 0, {0, 0}}
        :left -> {true, -1 * :math.pi() / 2, {0, win_h}}
        :right -> {true, :math.pi() / 2, {win_w, 0}}
        :upside_down -> {true, :math.pi(), {win_w, win_h}}
      end

    tx =
      cond do
        opts[:scaled] && centered && rotated ->
          Scenic.Math.Matrix.identity()
          |> Matrix.translate(center_trans)
          |> Matrix.translate(pin)
          |> Matrix.rotate(rot_angle)
          |> Matrix.scale(scale)
          |> Matrix.translate(inv_pin)

        centered && rotated ->
          Scenic.Math.Matrix.identity()
          |> Matrix.translate(center_trans)
          |> Matrix.translate(pin)
          |> Matrix.rotate(rot_angle)
          |> Matrix.translate(inv_pin)

        opts[:scaled] && centered ->
          Scenic.Math.Matrix.identity()
          |> Matrix.translate(center_trans)
          |> Matrix.translate(pin)
          |> Matrix.scale(scale)
          |> Matrix.translate(inv_pin)

        opts[:scaled] && rotated ->
          Scenic.Math.Matrix.identity()
          |> Matrix.translate(rot_trans)
          |> Matrix.rotate(rot_angle)
          |> Matrix.scale(scale)

        opts[:scaled] ->
          Scenic.Math.Matrix.identity()
          |> Matrix.scale(scale)

        centered ->
          Scenic.Math.Matrix.identity()
          |> Matrix.translate(center_trans)

        rotated ->
          Scenic.Math.Matrix.identity()
          |> Matrix.translate(rot_trans)
          |> Matrix.rotate(rot_angle)

        true ->
          Scenic.Math.Matrix.identity()
      end

    rel_tx =
      cond do
        opts[:scaled] && rotated ->
          Scenic.Math.Matrix.identity()
          |> Matrix.rotate(rot_angle)
          |> Matrix.scale(scale)

        opts[:scaled] ->
          Scenic.Math.Matrix.identity()
          |> Matrix.scale(scale)

        rotated ->
          Scenic.Math.Matrix.identity()
          |> Matrix.rotate(rot_angle)

        true ->
          Scenic.Math.Matrix.identity()
      end

    # tell the renderer to use the new global tx
    ToPort.set_global_tx(tx, port)

    # compute the inverse, or input, transform
    inv_tx = Matrix.invert(tx)
    inv_rel_tx = Matrix.invert(rel_tx)

    # sending a reshape input depends on the chosen configuration
    if !(opts[:scaled] || opts[:centered]) do
      send_input(driver, {:viewport, {:reshape, {win_w, win_h}}})
    end

    assign(driver, tx: tx, inv_tx: inv_tx, rel_tx: rel_tx, inv_rel_tx: inv_rel_tx)
  end
end
