defmodule Scenic.Driver.Local.CloseProtocolTest do
  use ExUnit.Case, async: true

  alias Scenic.Driver.Local.FromPort

  def close_callback(reason, owner, result) do
    send(owner, {:close_callback, reason})
    result
  end

  test "MFA receives reason before configured args and can defer" do
    driver = %Scenic.Driver{
      viewport: self(),
      assigns: %{on_close: {__MODULE__, :close_callback, [self(), :defer]}, closing: true}
    }

    message = <<0::unsigned-integer-size(32)-native, 7::unsigned-integer-size(32)-native>>
    assert {:noreply, updated} = FromPort.handle_port_message(message, driver)
    assert_receive {:close_callback, 7}
    refute updated.assigns.closing
  end

  test "MFA can cancel and reset the closing latch" do
    driver = %Scenic.Driver{
      viewport: self(),
      assigns: %{on_close: {__MODULE__, :close_callback, [self(), :cancel]}, closing: true}
    }

    message = <<0::unsigned-integer-size(32)-native, 0::unsigned-integer-size(32)-native>>
    assert {:noreply, updated} = FromPort.handle_port_message(message, driver)
    refute updated.assigns.closing
  end
end
