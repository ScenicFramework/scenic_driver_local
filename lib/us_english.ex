#
#  Created by Boyd Multerer on 2021-09-08
#  Copyright 2021 Kry10 Limited
#

defmodule Scenic.KeyMap.USEnglish do
  @moduledoc """
  Behaviour and support for mapping key press/repeat input to codepoint strings.
  """

  @behaviour Scenic.Driver.KeyMap
  alias Scenic.Driver.KeyMap

  @unshift_char_map %{
    key_apostrophe: "'",
    key_comma: ",",
    key_minus: "-",
    key_dot: ".",
    key_slash: "/",
    key_space: " ",
    key_0: "0",
    key_1: "1",
    key_2: "2",
    key_3: "3",
    key_4: "4",
    key_5: "5",
    key_6: "6",
    key_7: "7",
    key_8: "8",
    key_9: "9",
    key_semicolon: ";",
    key_equal: "=",
    key_a: "a",
    key_b: "b",
    key_c: "c",
    key_d: "d",
    key_e: "e",
    key_f: "f",
    key_g: "g",
    key_h: "h",
    key_i: "i",
    key_j: "j",
    key_k: "k",
    key_l: "l",
    key_m: "m",
    key_n: "n",
    key_o: "o",
    key_p: "p",
    key_q: "q",
    key_r: "r",
    key_s: "s",
    key_t: "t",
    key_u: "u",
    key_v: "v",
    key_w: "w",
    key_x: "x",
    key_y: "y",
    key_z: "z",
    key_leftbrace: "[",
    key_rightbrace: "]",
    key_backslash: "\\",
    key_grave: "`"
  }

  @caps_lock_map %{
    key_apostrophe: "'",
    key_comma: ",",
    key_minus: "-",
    key_dot: ".",
    key_slash: "/",
    key_space: " ",
    key_0: "0",
    key_1: "1",
    key_2: "2",
    key_3: "3",
    key_4: "4",
    key_5: "5",
    key_6: "6",
    key_7: "7",
    key_8: "8",
    key_9: "9",
    key_semicolon: ";",
    key_equal: "=",
    key_a: "A",
    key_b: "B",
    key_c: "C",
    key_d: "D",
    key_e: "E",
    key_f: "F",
    key_g: "G",
    key_h: "H",
    key_i: "I",
    key_j: "J",
    key_k: "K",
    key_l: "L",
    key_m: "M",
    key_n: "N",
    key_o: "O",
    key_p: "P",
    key_q: "Q",
    key_r: "R",
    key_s: "S",
    key_t: "T",
    key_u: "U",
    key_v: "V",
    key_w: "W",
    key_x: "X",
    key_y: "Y",
    key_z: "Z",
    key_leftbrace: "[",
    key_rightbrace: "]",
    key_backslash: "\"",
    key_grave: "`"
  }

  @shift_char_map %{
    key_apostrophe: "\"",
    key_comma: "<",
    key_minus: "_",
    key_dot: ">",
    key_slash: "?",
    key_space: " ",
    key_0: ")",
    key_1: "!",
    key_2: "@",
    key_3: "#",
    key_4: "$",
    key_5: "%",
    key_6: "^",
    key_7: "&",
    key_8: "*",
    key_9: "(",
    key_semicolon: ":",
    key_equal: "+",
    key_a: "A",
    key_b: "B",
    key_c: "C",
    key_d: "D",
    key_e: "E",
    key_f: "F",
    key_g: "G",
    key_h: "H",
    key_i: "I",
    key_j: "J",
    key_k: "K",
    key_l: "L",
    key_m: "M",
    key_n: "N",
    key_o: "O",
    key_p: "P",
    key_q: "Q",
    key_r: "R",
    key_s: "S",
    key_t: "T",
    key_u: "U",
    key_v: "V",
    key_w: "W",
    key_x: "X",
    key_y: "Y",
    key_z: "Z",
    key_leftbrace: "{",
    key_rightbrace: "}",
    key_backslash: "|",
    key_grave: "~"
  }

  @impl KeyMap
  def map_key(key, _value, keys, state) do
    cap_lock? = KeyMap.caps_lock?(keys)
    shift? = KeyMap.shift?(keys)

    cond do
      shift? -> {:ok, Map.get(@shift_char_map, key), state}
      cap_lock? -> {:ok, Map.get(@caps_lock_map, key), state}
      true -> {:ok, Map.get(@unshift_char_map, key), state}
    end
  end
end
