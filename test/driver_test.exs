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
    assert {:error, validation_error} = Scenic.Driver.Local.validate_opts(name: 'Bob')

    assert validation_error.message =~ "expected :name"
  end
end
