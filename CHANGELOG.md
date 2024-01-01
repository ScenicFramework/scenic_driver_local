## 0.12
This is a major update that adds support for Cairo and makes Cairo the default renderer:
* Added Cairo support - @ringlej
* Restructured C code - @ringlej

Note: for Nerves setups you may need additional configuration for your display
to work as expected. If you're using an RPI3 you may want to add an `erlinit`
configuration so that your IEx output isn't displayed on the screen. To do so in your `config/target.exs` add:
# From: https://hexdocs.pm/nerves/1.10.5/connecting-to-a-nerves-target.html#hdmi-cable
`config :nerves, :erlinit, ctty: "ttyAMA0"`

## 0.11.0
Very minor fixes

## 0.11.0-beta.0
This is the first public release to hex. Requires the v0.11 branch of scenic.
Documentation is spotty and is mostly in the README.md file.
