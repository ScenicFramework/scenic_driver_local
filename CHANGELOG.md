## 0.12.0-rc.0
This is a major update that adds support for Cairo and makes Cairo the default renderer:

* Add Cairo support - @ringlej
* Restructure C code - @ringlej
* Add CI - @axelson
* Updat README for 0.12 - @axelson
* Improve text handling - @JediLuke
* Better control of scene element opacity with `:clear` background - @seb3s
* Add `input_blacklist` options to remove `InputEvent` streams - @ringlej
* Fix `script_opts_draw_line` to handle stroke properly - @ringlej
* Add arc script command - @GPrimola
* Fix `LINE_CAP_ROUND -> LINE_JOIN_ROUND` in nanovg backend - @ringlej
* Fix extra `free` in image code - @ringlej
* Fix hidden cursor on refocus and repaint for nanovg backend - @mneumann
* Fix readme typo - @seb3s
* Add FPS and debug logging - @ringlej
* Add alpha channel support for sprites - @seb3s
* Add support for rounded rectangles - @GPrimola
* Add configurable framebuffer device for cairo - @jimsynz
* Assorted updates for deprecations and warnings





Note: for Nerves setups you may need additional configuration for your display
to work as expected. If you're using an RPI3 you may want to add an `erlinit`
configuration so that your IEx output isn't displayed on the screen. To do so in your `config/target.exs` add:
```
# From: https://hexdocs.pm/nerves/1.10.5/connecting-to-a-nerves-target.html#hdmi-cable
`config :nerves, :erlinit, ctty: "ttyAMA0"`
```



## 0.11.0
Very minor fixes

## 0.11.0-beta.0
This is the first public release to hex. Requires the v0.11 branch of scenic.
Documentation is spotty and is mostly in the README.md file.
