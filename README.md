# # scenic_driver_local

This is intended to be the main driver for rendering [Scenic](https://github.com/boydm/scenic) on a local computer. For now it only works on RPI3 and below under Nerves.


## Installation

In your Nerves applications dependencies include the following line

    ...
    {:scenic_driver_local, github: "ScenicFramework/scenic_driver_local"}
    ...

NOTE: For now you also have to use the scenic "input_and_drivers" branch

## Configuration

Configure the rpi driver the same way you configure other drivers. Add it
to the driver list in your ViewPort's config.exs file.

    config :my_app, :viewport,
      name: :main_viewport,
      size: {800, 480},
      default_scene: {Sample.Scene.Simple, nil},
      drivers: [
        [
          module: Scenic.Driver.Local
        ]
      ]


