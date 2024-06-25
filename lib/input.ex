#
#  Created by Boyd Multerer on 2021-09-04.
#  Copyright 2018-2021 Kry10 Limited
#

defmodule Scenic.Driver.Local.Input do
  @moduledoc false

  require Logger
  alias Scenic.Driver
  alias Scenic.Math.Vector2
  alias Scenic.Driver.KeyMap
  alias Scenic.Driver.Local.Cursor

  import Driver,
    only: [
      # get: 3,
      assign: 2,
      assign: 3,
      send_input: 2
    ]

  @identity_calibration {{1, 0, 0}, {0, 1, 0}}

  # this should be long enough to get rid of complete repeats,
  # but short enough to be below the key repeat timer
  @debounce_ms 10

  defp calibrate(name, calibrations) do
    calibrations
    |> Enum.find(fn {n, _} -> n == name end)
    |> case do
      nil -> @identity_calibration
      {_, calibration} -> calibration
    end
  end

  # --------------------------------------------------------
  def init_input(%{assigns: %{calibration: calibrations}} = driver) do
    # input_state = InputEvent.enumerate()
    inputs = InputEvent.enumerate()

    # Some input sources (microsoft keyboard) register themselves twice.

    input_state =
      inputs
      |> Enum.reject(fn {_, info} ->
        Enum.find(driver.assigns.input_blacklist, &(&1 == info.name))
      end)
      |> Enum.reduce(%{}, fn {path, info}, acc ->
        {:ok, pid} = InputEvent.start_link(path)

        # Logger.info "#{inspect(__MODULE__)}: #{inspect({path, info.name})}"

        Map.put(acc, path, %{
          pid: pid,
          # info: info,
          abs_pos: {-1, -1},
          calibration: calibrate(info.name, calibrations),
          debounce: []
        })
      end)

    assign(driver, input_state: input_state)
  end

  # --------------------------------------------------------
  def clear_input_debounce(source, event, %{assigns: %{input_state: input_state}} = driver) do
    case Map.fetch(input_state, source) do
      {:ok, %{debounce: debounce} = source_state} ->
        debounce = Enum.reject(debounce, &Kernel.==(&1, event))
        source_state = Map.put(source_state, :debounce, debounce)
        input_state = Map.put(input_state, source, source_state)
        assign(driver, :input_state, input_state)

      _ ->
        driver
    end
  end

  # --------------------------------------------------------
  # device was disconnected
  def handle_input(path, :disconnect, %{assigns: %{input_state: inputs}} = driver) do
    inputs = Map.delete(inputs, path)
    {:noreply, assign(driver, input_state: inputs)}
  end

  # --------------------------------------------------------
  def handle_input(
        source,
        events,
        %{
          assigns: %{
            input_state: input_state,
            cursor_pos: old_cursor_pos,
            auto_cursor: auto_cursor
          }
        } = driver
      ) do
    # Logger.warn( "INPUT #{source}: #{inspect(events)}" )

    driver =
      case Map.fetch(input_state, source) do
        {:ok, device} ->
        # record the old pos and key state
        %{abs_pos: old_abs_pos} = device

        # add some zero'd out relative values to the device state
        device =
          device
          |> Map.put(:rel_pos_dx, 0)
          |> Map.put(:rel_pos_dy, 0)
          |> Map.put(:rel_wheel_dx, 0)
          |> Map.put(:rel_wheel_dy, 0)

        # process the events
        {device, driver} =
          Enum.reduce(events, {device, driver}, fn event, {%{debounce: db}, _} = acc ->
            case Enum.member?(db, event) do
              true -> acc
              false -> do_input_event(event, source, acc)
            end
          end)

        # if the absolute position changed (example: by a touchscreen), then calculate the
        # normal (not transformed) screen coordinates and move that into the cursor_pos
        new_abs_pos = device[:abs_pos]

        driver =
          case new_abs_pos != old_abs_pos do
            true -> assign(driver, :cursor_pos, new_abs_pos)
            false -> driver
          end

        # parse rest if the state
        %{
          rel_wheel_dx: rel_wheel_dx,
          rel_wheel_dy: rel_wheel_dy
        } = device

        %{
          assigns: %{
            cursor_pos: new_cursor_pos,
            keys: new_keys
          }
        } = driver

        # calculate the cursor position in scene coordinates
        scene_pos = scene_coords(new_cursor_pos, device, driver)

        # the regular cursor_scroll event
        if rel_wheel_dx != 0 || rel_wheel_dy != 0 do
          # vector = scene_rel({rel_wheel_dx, rel_wheel_dy}, device, driver)
          vector = {rel_wheel_dx, rel_wheel_dy}
          send_input(driver, {:cursor_scroll, {vector, scene_pos}})
        end

        # The cursor itself would have changed if absolute position devices were in use
        driver =
          with true <- auto_cursor,
               true <- new_cursor_pos != old_cursor_pos do
            send_input(driver, {:cursor_pos, scene_pos})

            driver
            |> Cursor.hide()
            |> Cursor.set_position(new_cursor_pos)
          else
            _ -> driver
          end

        # generate the cursor button events
        # we didn't generate them at the time the button was pressed because the x/y position
        # changes are often after them in the event list. So... process the event first then
        # rescan the events looking for any button events that would trigger an input
        Enum.each(events, fn
          {:ev_key, key, value} ->
            with "btn_" <> _ <- to_string(key) do
              # special case btn_touch to be a left mouse button press
              {btn, driver} =
                case key do
                  :btn_touch -> {:btn_left, Cursor.hide(driver)}
                  btn -> {btn, driver}
                end

              send_input(driver, {:cursor_button, {btn, value, KeyMap.mods(new_keys), scene_pos}})
            end

          _ ->
            :ok
        end)

        # clean up the temporary relative device state
        device =
          device
          |> Map.delete(:rel_pos_dx)
          |> Map.delete(:rel_pos_dy)
          |> Map.delete(:rel_wheel_dx)
          |> Map.delete(:rel_wheel_dy)

        # save the completed device state in the input state
        input_state = Map.put(input_state, source, device)
        assign(driver, input_state: input_state)

        _ -> driver
      end

    {:noreply, driver}
  end

  defp do_input_event({:ev_abs, :abs_x, x}, _, {%{abs_pos: {_, y}} = device, driver}) do
    device = Map.put(device, :abs_pos, {x, y})
    {device, driver}
  end

  defp do_input_event({:ev_abs, :abs_y, y}, _, {%{abs_pos: {x, _}} = device, driver}) do
    device = Map.put(device, :abs_pos, {x, y})
    {device, driver}
  end

  defp do_input_event({:ev_rel, :rel_x, dx}, _, {
         device,
         %{assigns: %{rel_x: old_dx}} = driver
       }) do
    driver =
      driver
      |> assign(rel_x: old_dx + dx, cursor_update: true)
      |> Driver.request_update()

    {device, driver}
  end

  defp do_input_event({:ev_rel, :rel_y, dy}, _, {
         device,
         %{assigns: %{rel_y: old_dy}} = driver
       }) do
    driver =
      driver
      |> assign(rel_y: old_dy + dy, cursor_update: true)
      |> Driver.request_update()

    {device, driver}
  end

  defp do_input_event({:ev_rel, :rel_hwheel, dx}, _, {%{rel_wheel_dx: old_dx} = device, driver}) do
    device = Map.put(device, :rel_wheel_dx, old_dx + dx)
    {device, driver}
  end

  defp do_input_event({:ev_rel, :rel_wheel, dy}, _, {%{rel_wheel_dy: old_dy} = device, driver}) do
    device = Map.put(device, :rel_wheel_dy, old_dy + dy)
    {device, driver}
  end

  defp do_input_event({:ev_led, key, value}, _, {device, %{assigns: %{keys: old_keys}} = driver}) do
    send_input(driver, {:led, {key, value}})
    new_keys = Map.put(old_keys, key, value)
    driver = assign(driver, :keys, new_keys)
    {device, driver}
  end

  defp do_input_event({:ev_sw, key, value}, _, {device, %{assigns: %{keys: old_keys}} = driver}) do
    send_input(driver, {:switch, {key, value}})
    new_keys = Map.put(old_keys, key, value)
    driver = assign(driver, :keys, new_keys)
    {device, driver}
  end

  # special case the caps lock key as we need to track it's state ourselves
  defp do_input_event(
         {:ev_key, :key_capslock, 1} = event,
         source,
         {
           %{debounce: db} = device,
           %{assigns: %{keys: old_keys}} = driver
         }
       ) do
    value =
      case Map.get(old_keys, :virt_caps_lock, 0) do
        0 -> 1
        _ -> 0
      end

    new_keys =
      old_keys
      |> Map.put(:key_capslock, 1)
      |> Map.put(:virt_caps_lock, value)

    driver = assign(driver, :keys, new_keys)
    device = Map.put(device, :debounce, [event | db])
    Process.send_after(self(), {:_clear_input_debounce_, source, event}, @debounce_ms)
    mods = KeyMap.mods(new_keys)
    send_input(driver, {:key, {:key_capslock, 1, mods}})
    send_input(driver, {:key, {:virt_caps_lock, value, mods}})
    {device, driver}
  end

  # special case the nums lock key as we need to track it's state ourselves
  defp do_input_event(
         {:ev_key, :key_numlock, 1} = event,
         source,
         {
           %{debounce: db} = device,
           %{assigns: %{keys: old_keys}} = driver
         }
       ) do
    value =
      case Map.get(old_keys, :virt_num_lock, 0) do
        0 -> 1
        _ -> 0
      end

    new_keys =
      old_keys
      |> Map.put(:key_numlock, 1)
      |> Map.put(:virt_num_lock, value)

    driver = assign(driver, :keys, new_keys)
    device = Map.put(device, :debounce, [event | db])
    Process.send_after(self(), {:_clear_input_debounce_, source, event}, @debounce_ms)
    mods = KeyMap.mods(new_keys)
    send_input(driver, {:key, {:key_numlock, 1, mods}})
    send_input(driver, {:key, {:virt_num_lock, value, mods}})
    {device, driver}
  end

  # special case the nums lock key as we need to track it's state ourselves
  defp do_input_event(
         {:ev_key, :key_scrolllock, 1} = event,
         source,
         {
           %{debounce: db} = device,
           %{assigns: %{keys: old_keys}} = driver
         }
       ) do
    value =
      case Map.get(old_keys, :virt_scroll_lock, 0) do
        0 -> 1
        _ -> 0
      end

    new_keys =
      old_keys
      |> Map.put(:key_scrolllock, 1)
      |> Map.put(:virt_scroll_lock, value)

    driver = assign(driver, :keys, new_keys)
    device = Map.put(device, :debounce, [event | db])
    Process.send_after(self(), {:_clear_input_debounce_, source, event}, @debounce_ms)
    mods = KeyMap.mods(new_keys)
    send_input(driver, {:key, {:key_scrolllock, 1, mods}})
    send_input(driver, {:key, {:virt_scroll_lock, value, mods}})
    {device, driver}
  end

  # key strokes are complicated because they may or may not also trigger a codepoint event
  # they also need to be de-bounced as keyboards seem to send them multiple times
  defp do_input_event(
         {:ev_key, key, value} = event,
         source,
         {
           %{debounce: db} = device,
           %{
             assigns: %{
               keys: old_keys,
               key_map: mapper,
               mapper_state: mapper_state
             }
           } = driver
         }
       ) do
    # Logger.warn( "put key: #{inspect({key, value})}, keys: #{inspect(device.keys)}" )

    new_keys = Map.put(old_keys, key, value)
    driver = assign(driver, :keys, new_keys)
    device = Map.put(device, :debounce, [event | db])
    Process.send_after(self(), {:_clear_input_debounce_, source, event}, @debounce_ms)

    send_input(driver, {:key, {key, value, KeyMap.mods(new_keys)}})

    # send codepoint if appropriate
    driver =
      case mapper.map_key(key, value, new_keys, mapper_state) do
        {:ok, nil, state} ->
          assign(driver, :mapper_state, state)

        {:ok, cp, state} when is_bitstring(cp) ->
          case value do
            0 ->
              driver

            _ ->
              send_input(driver, {:codepoint, {cp, KeyMap.mods(new_keys)}})

              driver
              |> Cursor.hide()
              |> assign(:mapper_state, state)
          end

        {:error, msg, state} when is_bitstring(msg) ->
          Logger.error("#{inspect(mapper)}: " <> msg)
          assign(driver, :mapper_state, state)
      end

    {device, driver}
  end

  defp do_input_event(_event, _driver, acc), do: acc

  defp scene_coords(
         {x, y},
         %{calibration: {{ax, bx, dx}, {ay, by, dy}}},
         %{assigns: %{inv_tx: inv_tx}}
       ) do
    # calculate the calibrated raw position in screen coordinates
    {
      x * ax + y * bx + dx,
      x * ay + y * by + dy
    }
    |> Vector2.project(inv_tx)
  end
end
