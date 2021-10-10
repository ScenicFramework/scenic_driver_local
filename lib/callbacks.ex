#
#  Created by Boyd Multerer on 2021-09-04.
#  Copyright 2018-2021 Kry10 Limited
#

defmodule Scenic.Driver.Local.Calbacks do
  @moduledoc false

  alias Scenic.Driver
  alias Scenic.Script
  alias Scenic.ViewPort
  alias Scenic.Assets.Static
  alias Scenic.Assets.Stream
  alias Scenic.Math.Vector2

  alias Scenic.Driver.Local.ToPort

  import Driver,
    only: [
      assign: 2,
      assign: 3,
      set_busy: 2,
      send_input: 2
    ]

  # same as scenic/script.ex
  # @root_id Scenic.ViewPort.root_id()

  # --------------------------------------------------------
  @doc false
  def reset_scene(%{assigns: %{port: port, media: media}} = driver) do
    Stream.unsubscribe(:all)
    ToPort.reset_start(port)
    send(self(), {:_set_cursor_, :touch_spot})

    # state changes
    fonts = Map.get(media, :fonts, [])
    driver = assign(driver, :media, %{fonts: fonts})
    {:ok, driver}
  end

  # --------------------------------------------------------
  @doc false
  def update_scene(
        ids,
        %{
          assigns: %{
            cursor_update: true,
            rel_x: 0,
            rel_y: 0,
            cursor_pos: pos,
            cursor_showing: show?,
            port: port
          }
        } = driver
      ) do
    ToPort.update_cursor(show?, pos, port)
    do_update_scene(ids, driver)
  end

  def update_scene(
        ids,
        %{
          assigns: %{
            cursor_update: true,
            rel_x: dx,
            rel_y: dy,

            # tx: tx,
            inv_tx: inv_tx,
            rel_tx: rel_tx,
            # inv_rel_tx: inv_rel_tx,

            cursor_pos: {cx, cy},
            window_size: {width, height},
            # logical_size: {width, height},
            port: port
          }
        } = driver
      ) do
    # project the dx/dy vector so it points in the right direction
    {dx, dy} = Vector2.project({dx, dy}, rel_tx)

    # compute the new cursor position
    {cx, cy} = {cx + dx, cy + dy}

    # limit it to the screen
    cx =
      cond do
        cx < 0 -> 0
        cx >= width -> width - 1
        true -> cx
      end

    cy =
      cond do
        cy < 0 -> 0
        cy >= height -> height - 1
        true -> cy
      end

    screen_pos = {cx, cy}

    # convert the point from screen coords to scene coords
    scene_pos = Vector2.project(screen_pos, inv_tx)

    driver =
      driver
      |> send_input({:relative, {dx, dy}})
      |> send_input({:cursor_pos, scene_pos})
      |> assign(cursor_showing: true, cursor_pos: screen_pos)

    ToPort.update_cursor(true, scene_pos, port)

    # Finally, update the scripts
    do_update_scene(ids, driver)
  end

  def update_scene(ids, driver), do: do_update_scene(ids, driver)

  defp do_update_scene(ids, %{assigns: %{port: port, dirty_streams: streams}} = driver) do

    # update any pending streams
    streams
    |> Enum.uniq()
    |> Enum.each( &do_put_stream(&1, port) )

    driver =
      driver
      |> do_put_scripts(ids)
      |> assign(
        cursor_update: false,
        rel_x: 0,
        rel_y: 0,
        dirty_streams: []
      )
      |> set_busy(true)

    ToPort.render(port)
    {:ok, driver}
  end

  # --------------------------------------------------------
  @doc false
  def del_scripts(ids, %{assigns: %{port: port}} = driver) do
    Enum.each(ids, &ToPort.del_script(&1, port))
    {:ok, driver}
  end

  # --------------------------------------------------------
  @doc false
  def clear_color(color, %{assigns: %{port: port}} = driver) do
    ToPort.clear_color(color, port)
    {:ok, driver}
  end

  # ============================================================================
  # message handlers

  # --------------------------------------------------------
  # streaming asset updates
  defp do_put_stream( id, port ) do
    case Stream.fetch(id) do
      {:ok, {Stream.Image, {w, h, _mime}, bin}} ->
        ToPort.put_texture(port, id, :file, w, h, bin)

      {:ok, {Stream.Bitmap, {w, h, type}, bin}} ->
        ToPort.put_texture(port, id, type, w, h, bin)

      _ -> :ok
    end
  end

  # defp do_put_stream(Stream.Bitmap, id, %{assigns: %{port: port}} = driver) do
  #   driver = case Stream.fetch(id) do
  #     {:ok, {Stream.Bitmap, {w, h, type}, bin}} ->
  #     ToPort.put_texture(port, id, type, w, h, bin)
  #       Driver.request_update(driver)
  #     _ -> driver
  #   end
  #   {:noreply, driver}
  # end

  # def handle_put_stream(_, _id, driver), do: {:noreply, driver}

  # ============================================================================
  # rendering specific functions

  # --------------------------------------------------------
  defp do_put_scripts(%{assigns: %{port: port}, viewport: vp} = driver, ids) do
    Enum.reduce(ids, driver, fn id, driver ->
      with {:ok, script} <- ViewPort.get_script(vp, id) do
        driver = ensure_media(script, driver)

        script
        |> Script.serialize()
        |> ToPort.put_script(id, port)

        driver
      else
        _ -> driver
      end
    end)
  end

  defp ensure_media(script, driver) do
    media = Script.media(script)

    driver
    |> ensure_fonts(Map.get(media, :fonts, []))
    |> ensure_images(Map.get(media, :images, []))
    |> ensure_streams(Map.get(media, :streams, []))
  end

  defp ensure_fonts(driver, []), do: driver

  defp ensure_fonts(%{assigns: %{port: port, media: media}} = driver, ids) do
    fonts = Map.get(media, :fonts, [])

    fonts =
      Enum.reduce(ids, fonts, fn id, fonts ->
        with false <- Enum.member?(fonts, id),
             {:ok, {Static.Font, _}} <- Static.meta(id),
             {:ok, str_hash} <- Static.to_hash(id),
             {:ok, bin} <- Static.load(id) do
          ToPort.put_font(port, str_hash, bin)
          [id | fonts]
        else
          _ -> fonts
        end
      end)

    assign(driver, :media, Map.put(media, :fonts, fonts))
  end

  defp ensure_images(driver, []), do: driver

  defp ensure_images(%{assigns: %{port: port, media: media}} = driver, ids) do
    images = Map.get(media, :images, [])

    images =
      Enum.reduce(ids, images, fn id, images ->
        with false <- Enum.member?(images, id),
             {:ok, {Static.Image, {w, h, _}}} <- Static.meta(id),
             {:ok, str_hash} <- Static.to_hash(id),
             {:ok, bin} <- Static.load(id) do
          ToPort.put_texture(port, str_hash, :file, w, h, bin)
          [id | images]
        else
          _ -> images
        end
      end)

    assign(driver, :media, Map.put(media, :images, images))
  end

  defp ensure_streams(driver, []), do: driver

  defp ensure_streams(%{assigns: %{port: port, media: media}} = driver, ids) do
    streams = Map.get(media, :streams, [])

    streams =
      Enum.reduce(ids, streams, fn id, streams ->
        with false <- Enum.member?(streams, id),
             :ok <- Stream.subscribe(id) do
          case Stream.fetch(id) do
            {:ok, {Stream.Image, {w, h, _format}, bin}} ->
              ToPort.put_texture(port, id, :file, w, h, bin)
              [id | streams]

            {:ok, {Stream.Bitmap, {w, h, format}, bin}} ->
              ToPort.put_texture(port, id, format, w, h, bin)
              [id | streams]

            _err ->
              streams
          end
        else
          _ -> streams
        end
      end)

    assign(driver, :media, Map.put(media, :streams, streams))
  end
end
