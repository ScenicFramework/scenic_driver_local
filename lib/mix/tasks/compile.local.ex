defmodule Mix.Tasks.Compile.ScenicDriverLocal do
  use Mix.Task

  # import IEx

  @moduledoc """
  Automatically sets the SCENIC_LOCAL_TARGET for the Makefile
  """

  @mix_target (case function_exported?(Mix.Nerves.Utils, :mix_target, 0) do
                 true -> Mix.Nerves.Utils.mix_target()
                 false -> Mix.target()
               end)

  @spec target() :: atom
  def target(), do: @mix_target

  @spec run(OptionParser.argv()) :: {:ok, []} | no_return
  def run(_args) do
    # tell elixir_make which C target to build by setting a sys env var
    with nil <- System.get_env("SCENIC_LOCAL_TARGET") do
      case target() do
        n when n in [:dev, :host] ->
          System.put_env("SCENIC_LOCAL_TARGET", "cairo-gtk")

        _ ->
          System.put_env("SCENIC_LOCAL_TARGET", "cairo-fb")
      end
    end

    {:ok, []}
  end
end
