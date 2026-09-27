defmodule Scenic.Driver.Local.ToPortTest do
  use ExUnit.Case

  alias Scenic.Driver.Local.ToPort

  # cat echoes each packet back, so the test receives exactly
  # the bytes the C side would read
  setup do
    port = Port.open({:spawn, "cat"}, [:binary, {:packet, 4}])
    on_exit(fn -> if Port.info(port), do: Port.close(port) end)
    %{port: port}
  end

  test "put_script and del_script encode the id length the same way", %{port: port} do
    ToPort.put_script(<<>>, "abc", port)
    assert_receive({^port, {:data, <<0x01::32-native, 3::32-native, "abc">>}})

    ToPort.del_script("abc", port)
    assert_receive({^port, {:data, <<0x02::32-native, 3::32-native, "abc">>}})
  end
end
