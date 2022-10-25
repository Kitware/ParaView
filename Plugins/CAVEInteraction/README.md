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
