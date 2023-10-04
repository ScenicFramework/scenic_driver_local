#
#  Created by Boyd Multerer on 2021-09-04.
#  Copyright 2018-2021 Kry10 Limited
#

defmodule Scenic.Driver.Local.FromPort do
  @moduledoc false

  alias Scenic.ViewPort
  alias Scenic.Driver

  # import IEx

  require Logger

  import Bitwise

  import Scenic.Driver,
    only: [
      assign: 3,
      set_busy: 2,
      send_input: 2
    ]

  # incoming message ids
  @msg_close_id 0x00
  #  @msg_stats_id             0x01
  @msg_puts_id 0x02
  @msg_write_id 0x03
  @msg_inspect_id 0x04
  @msg_reshape_id 0x05
  @msg_ready_id 0x06

  @msg_info_id 0xA0
  @msg_warn_id 0xA1
  @msg_error_id 0xA2
  @msg_debug_id 0xA3

  # @msg_draw_ready_id 0x07

  @msg_key_id 0x0A
  @msg_char_id 0x0B
  @msg_cursor_pos_id 0x0C
  @msg_mouse_button_id 0x0D
  @msg_mouse_scroll_id 0x0E
  @msg_cursor_enter_id 0x0F

  # @msg_static_texture_miss 0x20
  # @msg_dynamic_texture_miss 0x21

  # @msg_font_miss 0x22
  # @msg_texture_miss 0x23

  @keymap_glfw 0x01
  @keymap_gdk 0x02

  # ============================================================================

  @doc false
  @spec handle_port_message(binary, Driver.t()) :: {:noreply, Driver.t()}
  def handle_port_message(msg, state)

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_ready_id::unsigned-integer-size(32)-native
        >>,
        driver
      ) do
    {:noreply, set_busy(driver, false)}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_reshape_id::unsigned-integer-size(32)-native,
          win_width::unsigned-integer-size(32)-native,
          win_height::unsigned-integer-size(32)-native
        >>,
        driver
      ) do
    driver =
      driver
      |> assign(:window_size, {win_width, win_height})
      |> Scenic.Driver.Local.set_global_tx()
      |> Driver.request_update()

    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_close_id::unsigned-integer-size(32)-native,
          reason::unsigned-integer-size(32)-native
        >>,
        %{assigns: %{on_close: on_close}, viewport: viewport} = driver
      ) do
    case on_close do
      :restart ->
        {:noreply, assign(driver, :closing, false)}

      :stop_driver ->
        ViewPort.stop_driver(viewport, self())
        {:noreply, assign(driver, :closing, true)}

      :stop_viewport ->
        ViewPort.stop(viewport)
        {:noreply, assign(driver, :closing, true)}

      :stop_system ->
        System.stop(reason)
        {:noreply, assign(driver, :closing, true)}

      :halt_system ->
        System.halt(reason)
        {:noreply, assign(driver, :closing, true)}

      {module, _fun, 1} ->
        module.fun(reason)
        {:noreply, assign(driver, :closing, true)}
    end
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<@msg_puts_id::unsigned-integer-size(32)-native>> <> msg,
        driver
      ) do
    Logger.info("scenic_driver_local puts: #{inspect(msg)}")
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<@msg_info_id::unsigned-integer-size(32)-native>> <> msg,
        driver
      ) do
    Logger.info("scenic_driver_local: #{msg}")
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<@msg_warn_id::unsigned-integer-size(32)-native>> <> msg,
        driver
      ) do
    Logger.warn("scenic_driver_local: #{msg}")
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<@msg_error_id::unsigned-integer-size(32)-native>> <> msg,
        driver
      ) do
    Logger.error("scenic_driver_local: #{msg}")
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<@msg_debug_id::unsigned-integer-size(32)-native>> <> msg,
        driver
      ) do
    Logger.debug("scenic_driver_local: #{msg}")
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<@msg_write_id::unsigned-integer-size(32)-native>> <> msg,
        driver
      ) do
    IO.write(msg)
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<@msg_inspect_id::unsigned-integer-size(32)-native>> <> msg,
        driver
      ) do
    # credo:disable-for-next-line Credo.Check.Warning.IoInspect
    IO.inspect(msg)
    Logger.info("scenic_driver_local inspect: #{inspect(msg)}")
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_key_id::unsigned-integer-size(32)-native,
          keymap::unsigned-integer-native-size(32),
          key::unsigned-integer-native-size(32),
          _scancode::unsigned-integer-native-size(32),
          action::integer-native-size(32),
          mods::unsigned-integer-native-size(32)
        >>,
        driver
      ) do
    key =
      case keymap do
        @keymap_glfw -> glfw_key_to_atom(key)
        @keymap_gdk -> gdk_key_to_atom(key)
        _ -> :key_unknown
      end

    mods =
      case keymap do
        @keymap_glfw -> glfw_prep_mods(mods)
        @keymap_gdk -> gdk_prep_mods(mods)
        _ -> []
      end

    send_input(driver, {:key, {key, action, mods}})

    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_char_id::unsigned-integer-size(32)-native,
          keymap::unsigned-integer-native-size(32),
          codepoint::unsigned-integer-native-size(32),
          mods::unsigned-integer-native-size(32)
        >>,
        driver
      ) do
    codepoint = codepoint_to_char(codepoint)

    mods =
      case keymap do
        @keymap_glfw -> glfw_prep_mods(mods)
        @keymap_gdk -> gdk_prep_mods(mods)
        _ -> []
      end

    send_input(driver, {:codepoint, {codepoint, mods}})

    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_cursor_pos_id::unsigned-integer-size(32)-native,
          x::float-native-size(32),
          y::float-native-size(32)
        >>,
        # state
        driver
      ) do
    pos = scene_coords({x, y}, driver)
    send_input(driver, {:cursor_pos, pos})
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_mouse_button_id::unsigned-integer-size(32)-native,
          keymap::unsigned-integer-native-size(32),
          button::unsigned-integer-native-size(32),
          action::unsigned-integer-native-size(32),
          mods::unsigned-integer-native-size(32),
          x::float-native-size(32),
          y::float-native-size(32)
        >>,
        driver
      ) do
    # action = action_to_atom(action)
    button =
      case keymap do
        @keymap_glfw -> glfw_button_to_atom(button)
        @keymap_gdk -> gdk_button_to_atom(button)
        _ -> :unknown
      end

    mods =
      case keymap do
        @keymap_glfw -> glfw_prep_mods(mods)
        @keymap_gdk -> gdk_prep_mods(mods)
        _ -> []
      end

    pos = scene_coords({x, y}, driver)
    send_input(driver, {:cursor_button, {button, action, mods, pos}})
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_mouse_scroll_id::unsigned-integer-size(32)-native,
          x_offset::float-native-size(32),
          y_offset::float-native-size(32),
          x_pos::float-native-size(32),
          y_pos::float-native-size(32)
        >>,
        driver
      ) do
    pos = scene_coords({x_pos, y_pos}, driver)
    input = {:cursor_scroll, {{x_offset, y_offset}, pos}}
    send_input(driver, input)
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_cursor_enter_id::unsigned-integer-size(32)-native,
          0::integer-native-size(32),
          x_pos::float-native-size(32),
          y_pos::float-native-size(32)
        >>,
        driver
      ) do
    pos = scene_coords({x_pos, y_pos}, driver)
    send_input(driver, {:viewport, {:exit, pos}})
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<
          @msg_cursor_enter_id::unsigned-integer-size(32)-native,
          1::integer-native-size(32),
          x_pos::float-native-size(32),
          y_pos::float-native-size(32)
        >>,
        driver
      ) do
    pos = scene_coords({x_pos, y_pos}, driver)
    send_input(driver, {:viewport, {:enter, pos}})
    {:noreply, driver}
  end

  # --------------------------------------------------------
  def handle_port_message(
        <<id::unsigned-integer-size(32)-native, bin::binary>>,
        driver
      ) do
    IO.puts("Unhandled port messages id: #{id}, msg: #{inspect(bin)}")
    {:noreply, driver}
  end

  # ============================================================================
  # utilities to translate GDK input to standardized input

  @gdk_button_atoms %{
    1 => :btn_left,
    2 => :btn_middle,
    3 => :btn_right
  }
  defp gdk_button_to_atom(code), do: Map.get(@gdk_button_atoms, code, :unknown)

  # ============================================================================
  # keyboard input helpers
  # these are for reading the keyboard directly. If you are trying to do text input
  # use the text/char helpers instead

  # --------------------------------------------------------

  # key codes use the standards defined by gdk, which generates them with 
  # https://gitlab.gnome.org/GNOME/gtk/-/blob/gtk-3-24/gdk/gdkkeysyms-update.pl
  # which sources it's values from
  # https://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h

  # --------------------------------------------------------
  @gdk_key_atoms %{
    32 => :key_space,
    39 => :key_apostrophe,
    44 => :key_comma,
    45 => :key_minus,
    46 => :key_dot,
    47 => :key_slash,
    48 => :key_0,
    49 => :key_1,
    50 => :key_2,
    51 => :key_3,
    52 => :key_4,
    53 => :key_5,
    54 => :key_6,
    55 => :key_7,
    56 => :key_8,
    57 => :key_9,
    59 => :key_semicolon,
    61 => :key_equal,
    65 => :key_a,
    66 => :key_b,
    67 => :key_c,
    68 => :key_d,
    69 => :key_e,
    70 => :key_f,
    71 => :key_g,
    72 => :key_h,
    73 => :key_i,
    74 => :key_j,
    75 => :key_k,
    76 => :key_l,
    77 => :key_m,
    78 => :key_n,
    79 => :key_o,
    80 => :key_p,
    81 => :key_q,
    82 => :key_r,
    83 => :key_s,
    84 => :key_t,
    85 => :key_u,
    86 => :key_v,
    87 => :key_w,
    88 => :key_x,
    89 => :key_y,
    90 => :key_z,
    91 => :key_leftbrace,
    92 => :key_backslash,
    93 => :key_rightbrace,
    96 => :key_grave,
    0xFF1B => :key_esc,
    0xFF0D => :key_enter,
    0xFF09 => :key_tab,
    0xFF08 => :key_backspace,
    0xFF63 => :key_insert,
    0xFFFF => :key_delete,
    0xFF53 => :key_right,
    0xFF51 => :key_left,
    0xFF54 => :key_down,
    0xFF52 => :key_up,
    0xFF55 => :key_pageup,
    0xFF56 => :key_pagedown,
    0xFF50 => :key_home,
    0xFF57 => :key_end,
    0xFFE5 => :key_capslock,
    0xFF14 => :key_scrolllock,
    0xFF7F => :key_numlock,
    0xFF61 => :key_screen,
    0xFF13 => :key_pause,
    0xFFBE => :key_f1,
    0xFFBF => :key_f2,
    0xFFC0 => :key_f3,
    0xFFC1 => :key_f4,
    0xFFC2 => :key_f5,
    0xFFC3 => :key_f6,
    0xFFC4 => :key_f7,
    0xFFC5 => :key_f8,
    0xFFC6 => :key_f9,
    0xFFC7 => :key_f10,
    0xFFC8 => :key_f11,
    0xFFC9 => :key_f12,
    0xFFCA => :key_f13,
    0xFFCB => :key_f14,
    0xFFCC => :key_f15,
    0xFFCD => :key_f16,
    0xFFCE => :key_f17,
    0xFFCF => :key_f18,
    0xFFD0 => :key_f19,
    0xFFD1 => :key_f20,
    0xFFD2 => :key_f21,
    0xFFD3 => :key_f22,
    0xFFD4 => :key_f23,
    0xFFD5 => :key_f24,
    0xFFD6 => :key_f25,
    0xFFB0 => :key_kp0,
    0xFFB1 => :key_kp1,
    0xFFB2 => :key_kp2,
    0xFFB3 => :key_kp3,
    0xFFB4 => :key_kp4,
    0xFFB5 => :key_kp5,
    0xFFB6 => :key_kp6,
    0xFFB7 => :key_kp7,
    0xFFB8 => :key_kp8,
    0xFFB9 => :key_kp9,
    0xFFAE => :key_kpdot,
    0xFFAF => :key_kpslash,
    0xFFAA => :key_kpasterisk,
    0xFFAD => :key_kpminus,
    0xFFAB => :key_kpplus,
    0xFF8D => :key_kpenter,
    0xFFBD => :key_kpequal,
    0xFFE1 => :key_leftshift,
    0xFFE3 => :key_leftctrl,
    0xFFE9 => :key_leftalt,
    # 0xffeb => "left_super"

    0xFFE2 => :key_rightshift,
    0xFFE4 => :key_rightctrl,
    0xFFEA => :key_rightalt,
    # 0xffec => "right_super"

    0xFF67 => :key_menu
  }
  defp gdk_key_to_atom(code), do: Map.get(@gdk_key_atoms, code, :key_unknown)

  # --------------------------------------------------------

  @gdk_mod_shift 1 <<< 0
  @gdk_mod_ctrl 1 <<< 2
  @gdk_mod_alt 1 <<< 3
  @gdk_mod_super 1 <<< 26
  @gdk_mod_caps_lock 1 <<< 1
  # @gdk_mod_num_lock 0x020
  defp gdk_prep_mods(mods) do
    []
    |> add_if_masked(mods, @gdk_mod_shift, :shift)
    |> add_if_masked(mods, @gdk_mod_ctrl, :ctrl)
    |> add_if_masked(mods, @gdk_mod_alt, :alt)
    |> add_if_masked(mods, @gdk_mod_super, :meta)
    |> add_if_masked(mods, @gdk_mod_caps_lock, :caps_lock)

    # |> add_if_masked(mods, @gdk_mod_num_lock, :num_lock)
  end

  # ============================================================================
  # utilities to translate Glfw input to standardized input

  @glfw_button_atoms %{
    0 => :btn_left,
    1 => :btn_right,
    2 => :btn_middle
  }
  defp glfw_button_to_atom(code), do: Map.get(@glfw_button_atoms, code, :unknown)

  # --------------------------------------------------------

  # key codes use the standards defined by Glfw
  # http://www.Glfw.org/docs/latest/group__keys.html

  # --------------------------------------------------------
  @glfw_key_atoms %{
    32 => :key_space,
    39 => :key_apostrophe,
    44 => :key_comma,
    45 => :key_minus,
    46 => :key_dot,
    47 => :key_slash,
    48 => :key_0,
    49 => :key_1,
    50 => :key_2,
    51 => :key_3,
    52 => :key_4,
    53 => :key_5,
    54 => :key_6,
    55 => :key_7,
    56 => :key_8,
    57 => :key_9,
    59 => :key_semicolon,
    61 => :key_equal,
    65 => :key_a,
    66 => :key_b,
    67 => :key_c,
    68 => :key_d,
    69 => :key_e,
    70 => :key_f,
    71 => :key_g,
    72 => :key_h,
    73 => :key_i,
    74 => :key_j,
    75 => :key_k,
    76 => :key_l,
    77 => :key_m,
    78 => :key_n,
    79 => :key_o,
    80 => :key_p,
    81 => :key_q,
    82 => :key_r,
    83 => :key_s,
    84 => :key_t,
    85 => :key_u,
    86 => :key_v,
    87 => :key_w,
    88 => :key_x,
    89 => :key_y,
    90 => :key_z,
    91 => :key_leftbrace,
    92 => :key_backslash,
    93 => :key_rightbrace,
    96 => :key_grave,
    256 => :key_esc,
    257 => :key_enter,
    258 => :key_tab,
    259 => :key_backspace,
    260 => :key_insert,
    261 => :key_delete,
    262 => :key_right,
    263 => :key_left,
    264 => :key_down,
    265 => :key_up,
    266 => :key_pageup,
    267 => :key_pagedown,
    268 => :key_home,
    269 => :key_end,
    280 => :key_capslock,
    281 => :key_scrolllock,
    282 => :key_numlock,
    283 => :key_screen,
    284 => :key_pause,
    290 => :key_f1,
    291 => :key_f2,
    292 => :key_f3,
    293 => :key_f4,
    294 => :key_f5,
    295 => :key_f6,
    296 => :key_f7,
    297 => :key_f8,
    298 => :key_f9,
    299 => :key_f10,
    300 => :key_f11,
    301 => :key_f12,
    302 => :key_f13,
    303 => :key_f14,
    304 => :key_f15,
    305 => :key_f16,
    306 => :key_f17,
    307 => :key_f18,
    308 => :key_f19,
    309 => :key_f20,
    310 => :key_f21,
    311 => :key_f22,
    312 => :key_f23,
    313 => :key_f24,
    314 => :key_f25,
    320 => :key_kp0,
    321 => :key_kp1,
    322 => :key_kp2,
    323 => :key_kp3,
    324 => :key_kp4,
    325 => :key_kp5,
    326 => :key_kp6,
    327 => :key_kp7,
    328 => :key_kp8,
    329 => :key_kp9,
    330 => :key_kpdot,
    331 => :key_kpslash,
    332 => :key_kpasterisk,
    333 => :key_kpminus,
    334 => :key_kpplus,
    335 => :key_kpenter,
    336 => :key_kpequal,
    340 => :key_leftshift,
    341 => :key_leftctrl,
    342 => :key_leftalt,
    # 343 => "left_super"

    344 => :key_rightshift,
    345 => :key_rightctrl,
    346 => :key_rightalt,
    # 347 => "right_super"

    348 => :key_menu
  }
  defp glfw_key_to_atom(code), do: Map.get(@glfw_key_atoms, code, :key_unknown)

  # --------------------------------------------------------
  @glfw_mod_shift 0x001
  @glfw_mod_ctrl 0x002
  @glfw_mod_alt 0x004
  @glfw_mod_super 0x008
  @glfw_mod_caps_lock 0x010
  @glfw_mod_num_lock 0x020
  defp glfw_prep_mods(mods) do
    []
    |> add_if_masked(mods, @glfw_mod_shift, :shift)
    |> add_if_masked(mods, @glfw_mod_ctrl, :ctrl)
    |> add_if_masked(mods, @glfw_mod_alt, :alt)
    |> add_if_masked(mods, @glfw_mod_super, :meta)
    |> add_if_masked(mods, @glfw_mod_caps_lock, :caps_lock)
    |> add_if_masked(mods, @glfw_mod_num_lock, :num_lock)
  end

  defp add_if_masked(list, mods, mask, key) do
    case Bitwise.&&&(mods, mask) do
      0 -> list
      _ -> [key | list]
    end
  end

  # --------------------------------------------------------
  defp codepoint_to_char(codepoint_to_atom)
  defp codepoint_to_char(cp), do: <<cp::utf8>>

  defp scene_coords({x, y}, %{assigns: %{inv_tx: inv_tx}}) do
    Scenic.Math.Vector2.project({x, y}, inv_tx)
  end
end
