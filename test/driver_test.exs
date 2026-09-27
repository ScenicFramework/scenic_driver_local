defmodule Scenic.Driver.LocalTest do
  use ExUnit.Case

  test "validate_opts/1 with valid opts" do
    opts = [
      name: :example_name,
      limit_ms: 100,
      layer: 0,
      opacity: 1,
      debug: false,
      debugger: "",
      debug_fps: 0,
      antialias: true,
      calibration: [{"calibration_name", {{0, 0, 0}, {0, 0, 0}}}],
      position: [
        scaled: false,
        centered: false,
        orientation: :normal
      ],
      window: [
        title: "Test Window",
        resizeable: false
      ],
      cursor: false,
      key_map: Scenic.KeyMap.USEnglish,
      on_close: :stop_system,
      input_blacklist: []
    ]

    assert Scenic.Driver.Local.validate_opts(opts) == {:ok, opts}
  end

  test "validate_opts/1 with invalid opts" do
    assert {:error, validation_error} = Scenic.Driver.Local.validate_opts(name: ~c"Bob")

    assert validation_error.message =~ "expected :name"
  end

  test "set_cursor/2 with a driver struct sends set_cursor to the driver" do
    Scenic.Driver.Local.set_cursor(%Scenic.Driver{pid: self()}, :touch_spot)
    assert_receive({:_set_cursor_, :touch_spot})
  end

  test "hide_cursor/1 with a driver struct sends hide_cursor to the driver" do
    Scenic.Driver.Local.hide_cursor(%Scenic.Driver{pid: self()})
    assert_receive(:_hide_cursor_)
  end

  test "the show_cursor message shows the cursor" do
    driver = %Scenic.Driver{assigns: %{cursor_showing: false}}

    assert {:noreply, %{assigns: %{cursor_showing: true}}} =
             Scenic.Driver.Local.handle_info(:_show_cursor_, driver)
  end
end
