# Scenic.Driver.Local

This is the main "local" renderer for Scenic applications.

If you are on a Mac/PC/Linux machine, or building under Nerves, then this is the driver to use to render on the screen connected to the computer Scenic is running on.

This driver replaces both `:scenic_driver_glfw` and `:scenic_driver_nerves_rpi` going forward for Scenic v0.11 and up.


## Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed
by adding `scenic_driver_local` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:scenic_driver_local, "~> 0.11.0"}
  ]
end
```

## Configuration

The configuration options for `scenic_driver_local` are slightly different from `scenic_driver_glfw`, so you may need to do a little work to bring over your options. Namely, options that pertain to the window under `scenic_driver_glfw`, aren't relevant to any Nerves build. So they are now under the `:window` option.

Example:

```elixir
[
  module: Scenic.Driver.Local,
  window: [title: "Local Window", resizeable: true]
]
```

There are quite a few new options as well. It uses `NimbleOptions` to confirm them, so look at the `Scenic.Driver.Local` module for details.

## Targets

This driver figures out what underlying graphics technology to use depending on what your MIX_TARGET is set to.

For example, for apps running on a Mac/PC/Linux, it is usually set to `host`, which causes the driver to use `cairo-gtk` as the underlying tech.

If you are building for Nerves, it will use `cairo-fb`

Previous versions of `scenic_driver_local` would use `bcm` (Broadcom Manager) for any of `rpi`, `rpi0`, `rip2`, `rpi3`, and `rpi3a` and `drm` for `bbb` and `rpi4`.
You can explicitly use thes by setting `SCENIC_LOCAL_TARGET=bcm` or `SCENIC_LOCAL_TARGET=drm`, **but these options are being deprecated**.
Please try the default of `SCENIC_LOCAL_TARGET=cairo-fb` as this should work universally on any Nerves target.

`cairo-fb` will require that your `nerves_system_*` has the `cairo` library selected.

## Prerequisites

This driver requires Scenic v0.11 or up.

### Installing on MacOS

You will need to install [XQuartz](https://www.xquartz.org/) on macOS.

The easiest way to install needed build time dependencies on MacOS is to use Homebrew. Just run the following in a terminal:

```bash
brew update
brew install gtk+3 cairo pkg-config
```

Once these components have been installed, you should be able to build the `scenic_driver_local` driver.

### Installing on Ubuntu

The easiest way to install on Ubuntu is to use apt-get. Just run the following:

```bash
apt-get update
apt-get install pkgconf libgtk-3-0 libgtk-3-dev libsystemd-dev libwebp-dev libzstd-dev
```

Once these components have been installed, you should be able to build the `scenic_driver_local` driver.

### Installing on Arch Linux

The easiest way to install on Arch Linux is to use pacman. Just run the following:


```bash
pacman -Syu
sudo pacman -S cairo gtk3
```

### Installing on Windows

**This section needs help updating for cairo support**

First, make sure to have installed Visual Studio with its "Desktop development with C++" package.

Next, you need to download the Windows binaries for [GLFW](https://www.glfw.org/download.html) and [GLEW](http://glew.sourceforge.net/index.html) manually.

Locate your Visual Studio installation directory. Two folders will be required for the next steps:

* The **Include** folder: `{Visual Studio Path}\VC\Tools\MSVC\{version number}\include`
* The **Lib** folder: `{Visual Studio Path}\VC\Tools\MSVC\{version number}\lib\x64`

Open the GLFW package you downloaded. Extract the contents of the packaged `include` folder to your Visual Studio **Include** folder. Next to the `include` folder, you'll also find several `lib-vc20xx` folders. Select the closest match to your Visual Studio version and extract the contents to your **Lib** folder.

Lastly, install the GLEW package. Find the packaged `include` folder and extract its contents to your **Include** folder as well. You should now have two new folders in your **Include** folder: `GL` and `GLFW`. Now navigate to `lib\Release\x64` in the GLEW package. Copy all `*.lib` files to your **Lib** folder. Finally, navigate to `bin\Release\x64` and copy `glew32.dll` to your `Windows\system32` folder.

Once these components have been installed, you should be able to build the `scenic_driver_local` driver.


## Documentation

Documentation can be found at [https://hexdocs.pm/scenic_driver_local](https://hexdocs.pm/scenic_driver_local).

