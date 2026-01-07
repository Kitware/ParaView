# CAVEInteraction Plugin

CAVEInteraction is a plugin to interact with Cave Automatic Virtual Environments

## Building the plugin

To enable this plugin, you need to provide the following cmake variable when configuring ParaView:
`-DPARAVIEW_PLUGIN_ENABLE_CAVEInteraction=ON`

`CAVEInteraction` plugin requires [VRPN](https://github.com/vrpn)
or/and [VRUI](https://web.cs.ucdavis.edu/~okreylos/ResDev/Vrui/index.html) to be installed, and provide one or both of
the following cmake variables when configuring ParaView:

1. `-DPARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN=ON`
2. `-DPARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI=ON`

## Using the first person camera interactor style

The usage of the `First Person Camera` interactor style is a bit different compared to the others : the controlled proxy must be a Render View and there is no controlled property since it definitely controls the camera. The input device must return the absolute position of the mouse on the screen as valuators (a valuator for X position and a valuator for Y position) to function correctly (a `vrpn_Mouse` for example with VRPN). In the CAVE, this interaction style won't have any effect unless the off-axis projection is turned off in the CAVE configuration.
