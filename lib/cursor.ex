#
#  Created by Boyd Multerer on 2021-09-06
#  Copyright 2018-2021 Kry10 Limited
#

# pre-compiled cursor scripts.

# the "hot point" is always {0, 0}, so orient around that.

defmodule Scenic.Driver.Local.Cursor do
  @moduledoc """
  Experimental library of cursors that you can use with a Nerves project.

  The `:cursor` option must be set to `true` in the driver config.

  Note: the implementation of cursors is pretty naive and will be much more
  performant in the future.

  In fact, I would really recommend using cursors at this point except to test
  it out and provide feedback...
  """

  alias Scenic.Driver
  alias Scenic.Script
  alias Scenic.Driver.Local.ToPort
  # alias Scenic.Driver.Local.Calbacks

  require Logger

  import Scenic.Driver,
    only: [
      assign: 2,
      assign: 3
    ]

  @type t ::
          :none
          | :crosshair
          | :pointer
          | :text
          | Script.t()

  @fill_color :black
  @stroke_width 3
  @stroke_color :white

  @cursor_script_id "_cursor_"

  @touch_spot Script.start()
              |> Script.push_state()
              |> Script.stroke_width(@stroke_width)
              |> Script.stroke_color(@stroke_color)
              |> Script.fill_color(@fill_color)
              |> Script.draw_circle(10, :stroke)
              |> Script.draw_circle(1, :stroke)
              |> Script.pop_state()
              |> Script.finish()
              |> Script.serialize()
              |> IO.iodata_to_binary()

  # @chx_script Script.start()
  #   |> Script.push_state()
  #   |> Script.join(:round)
  #   |> Script.stroke_width(@stroke_width)
  #   |> Script.stroke_color(@stroke_color)
  #   |> Script.begin_path()
  #   |> Script.move_to(0, 8)
  #   |> Script.line_to(5, 13)
  #   |> Script.line_to(12, 1)
  #   |> Script.stroke_path()
  #   |> Script.pop_state()
  #   |> Script.finish()
  #   |> Script.serialize()
  #   |> IO.iodata_to_binary()

  @doc false
  def set(%Driver{assigns: %{port: port}} = driver, :touch_spot) do
    ToPort.put_script(@touch_spot, @cursor_script_id, port)
    assign(driver, :cursor, :touch_spot)
  end

  @doc false
  def set_position(
        %{assigns: %{auto_cursor: true, cursor_showing: true}} = driver,
        pos
      ) do
    driver
    |> assign(cursor_pos: pos, cursor_update: true)
    |> Driver.request_update()
  end

  def set_position(driver, pos), do: assign(driver, :cursor_pos, pos)

  # --------------------------------------------------------
  @doc false
  def show(%{assigns: %{auto_cursor: true, cursor_showing: false}} = driver) do
    driver
    |> assign(cursor_showing: true, cursor_update: true)
    |> Driver.request_update()
  end

  def show(driver), do: assign(driver, :cursor_showing, true)

  # --------------------------------------------------------
  @doc false
  def hide(%{assigns: %{auto_cursor: true, cursor_showing: true}} = driver) do
    driver
    |> assign(cursor_showing: false, cursor_update: true)
    |> Driver.request_update()
  end

  def hide(driver), do: assign(driver, :cursor_showing, false)

  # --------------------------------------------------------

  # defp schedule_update(%{assigns: %{limit_cursor: true}} = driver), do: driver
  # defp schedule_update(%{limit_ms: limit_ms} = driver) do
  #   Process.send_after( self(), :_update_cursor_, limit_ms )
  #   assign( driver, :limit_cursor, true )
  # end

  # #--------------------------------------------------------
  # @doc false
  # def update(
  # %{
  #   # busy: false,
  #   limit_ms: limit_ms,
  #   assigns: %{
  #     cursor_showing: show?,
  #     cursor_pos: pos,
  #     port: port,
  #     last_cursor: last_cursor
  #   },
  #   # busy: false
  # } = driver ) do
  #   case {show?, pos} do
  #     ^last_cursor ->
  #       assign( driver, :limit_cursor, false )

  #     _ ->
  #       Process.send_after( self(), :_update_cursor_, limit_ms )
  #       ToPort.update_cursor( show?, pos, port )
  #       driver =
  #         driver
  #         |> assign( :limit_cursor, true )
  #         |> assign( :last_cursor, {show?, pos} )
  #         |> assign( :cursor_update, true )
  #         |> Driver.request_update()
  #   end
  # end
  # def update( driver ), do: driver
end
