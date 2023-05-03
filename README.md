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

This driver does its best to figure out what underlying graphics technology to use depending on what your MIX_TARGET is set to.

For example, for apps running on a Mac/PC/Linux, it is usually set to `host`, which causes the driver to use `glfw` as the underlying tech.

If you are building for Nerves, it will use `bcm` (Broadcom Manager) for any of `rpi`, `rpi0`, `rip2`, `rpi3`, and `rpi3a`.

### Nerves rpi4 & bbb Need Work / Help
`Scenic.Driver.Local` uses `drm` (Direct Render Manager) for the `rpi4` and `bbb`. It currently renders on the rpi4, but is __very slow__. I haven't figured out why yet and if anyone wants to dig in, that would be appreciated. It should be really fast, so is probably a hardware configuration issue.

The `bbb` doesn't work (is close in theory??) as the Nerves `bbb` system doesn't have the needed graphics support in it yet. There are others who have gotten SGX support working for the `bbb` and I could use some help from them.

### Custom Nerves Targets
For custom systems (example - figuring out how to add SGX support to the `bbb`) You will need to set SCENIC_LOCAL_TARGET manually. You may also need to set the SCENIC_LOCAL_GL as well.

example in your command line

```
export SCENIC_LOCAL_TARGET=drm
export SCENIC_LOCAL_GL=gles3
```

These options may change, especially as we sort out the issues on the `rpi4` and the `bbb`. If SCENIC_LOCAL_TARGET isn't set, then look in the build output for instructions.


## Prerequisites

This driver requires Scenic v0.11 or up.

### Installing on MacOS

The easiest way to install on MacOS is to use Homebrew. Just run the following in a terminal:

```bash
brew update
brew install glfw3 glew pkg-config
```


Once these components have been installed, you should be able to build the `scenic_driver_local` driver.

### Installing on Ubuntu 18.04

The easiest way to install on Ubuntu is to use apt-get. Just run the following:

```bash
apt-get update
apt-get install pkgconf libglfw3 libglfw3-dev libglew2.0 libglew-dev
```

Once these components have been installed, you should be able to build the `scenic_driver_local` driver.

### Installing on Ubuntu 20.04

The easiest way to install on Ubuntu is to use apt-get. Just run the following:

```bash
apt-get update
apt-get install pkgconf libglfw3 libglfw3-dev libglew2.2 libglew-dev
```

Once these components have been installed, you should be able to build the `scenic_driver_local` driver.

### Installing on Arch Linux

The easiest way to install on Arch Linux is to use pacman. Just run the following:


```bash
pacman -Syu
sudo pacman -S glfw-x11 glew
```

If you're using Wayland, you'll probably need `glfw-wayland` instead of `glfw-x11` and `glew-wayland` instead of `glew`

### Installing on Windows

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

