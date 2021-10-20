defmodule Scenic.Driver.Local.MixProject do
  use Mix.Project

  @app_name :scenic_driver_local
  @version "0.11.0-beta.0"
  @github "https://github.com/ScenicFramework/scenic_driver_local"

  def project do
    [
      app: @app_name,
      version: @version,
      elixir: "~> 1.12",
      start_permanent: Mix.env() == :prod,
      description: description(),
      build_embedded: true,
      compilers: compilers(),
      make_clean: ["clean"],
      make_targets: ["all"],
      make_env: make_env(),
      deps: deps(),
      package: package(),
      docs: [extras: ["README.md"]],
      xref: [exclude: [Mix.Nerves.Utils]]
    ]
  end

  # Run "mix help compile.app" to learn about applications.
  def application do
    [
      extra_applications: extra_applications()
    ]
  end

  def extra_applications do
    apps = [:logger]

    case Mix.env() do
      :dev -> apps ++ [:mix]
      _ -> apps
    end
  end

  # Run "mix help deps" to learn about dependencies.
  defp deps do
    [
      {:input_event, "~> 0.4"},
      {:scenic, "~> 0.11.0-beta.0"},
      {:elixir_make, "~> 0.6", runtime: false},
      {:ex_doc, ">= 0.0.0", only: :dev, runtime: false},
      {:dialyxir, "~> 1.1", only: :dev, runtime: false}
    ]
  end

  defp description() do
    """
    Scenic.Driver.Nerves - Scenic driver for Nerves devices
    """
  end

  defp make_env() do
    case System.get_env("ERL_EI_INCLUDE_DIR") do
      nil ->
        %{
          "ERL_EI_INCLUDE_DIR" => "#{:code.root_dir()}/usr/include",
          "ERL_EI_LIBDIR" => "#{:code.root_dir()}/usr/lib"
        }

      _ ->
        %{}
    end
  end

  defp compilers() do
    compilers(System.get_env("PUBLISH"))
  end

  defp compilers(nil) do
    Mix.compilers() ++ [:scenic_driver_local, :elixir_make]
  end

  defp compilers(_publish) do
    Mix.compilers()
  end

  defp package do
    [
      name: @app_name,
      contributors: ["Boyd Multerer"],
      maintainers: ["Boyd Multerer"],
      licenses: ["Apache 2"],
      links: %{Github: @github},
      files: [
        "Makefile",
        "LICENSE",
        # only include *.c and *.h files
        "c_src/**/*.[ch]",
        # only include *.ex files
        "lib/**/*.ex",
        "mix.exs"
      ]
    ]
  end
end
