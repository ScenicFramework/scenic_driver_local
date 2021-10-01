defmodule Mix.Tasks.Compile.ScenicDriverLocal do
  use Mix.Task

import IEx

  @moduledoc """
  Automatically sets the SCENIC_LOCAL_TARGET for the Makefile
  """

  @return if Version.match?(System.version(), "~> 1.9"), do: {:ok, []}, else: :ok


  @mix_target (case function_exported?(Mix.Nerves.Utils, :mix_target, 0) do
      true -> Mix.Nerves.Utils.mix_target()
      false -> Mix.target()
    end)


  @spec target() :: atom
  def target(), do: @mix_target

  @spec run(OptionParser.argv()) :: :ok | no_return
  def run(args) do
    # tell elixir_make which C target to build by setting a sys env var
    with nil <- System.get_env("SCENIC_LOCAL_TARGET") do
      case target() do
        :dev -> System.put_env("SCENIC_LOCAL_TARGET", "glfw")
        :host -> System.put_env("SCENIC_LOCAL_TARGET", "glfw")
        :rpi -> System.put_env("SCENIC_LOCAL_TARGET", "bcm")
        :rpi0 -> System.put_env("SCENIC_LOCAL_TARGET", "bcm")
        :rpi2 -> System.put_env("SCENIC_LOCAL_TARGET", "bcm")
        :rpi3 -> System.put_env("SCENIC_LOCAL_TARGET", "bcm")
        :rpi3a -> System.put_env("SCENIC_LOCAL_TARGET", "bcm")
        :bbb ->
          System.put_env("SCENIC_LOCAL_TARGET", "drm")
          System.put_env("SCENIC_LOCAL_GL", "gles2")

        _ ->
          System.put_env("SCENIC_LOCAL_TARGET", "drm")
          System.put_env("SCENIC_LOCAL_GL", "gles3")
      end
    end

    @return
  end

end