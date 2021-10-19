## Dynamic Initialization in Plugins

Some plugins need to invoke a function when they are loaded
rather than relying on static initialization.
Examples of this include:

+ calling third-party library initialization methods;
+ registering functionality in one plugin that will be
  used by functions in a separate plugin.

This can be accomplished via the `pqAutoStartInterface` in
Qt-based applications or via a python initializer when building
ParaView with Python; however, there was not a way to do this
robustly in all build configurations.

Now, the `add_paraview_plugin` macro accepts two new parameters:

+ `INITIALIZERS`, which accepts a list of free functions to invoke
  when the plugin is loaded; and
+ `EXTRA_INCLUDES`, which accepts a list of header files to include
  in the plugin's implementation. (This is useful for ensuring the
  free functions are declared before they are invoked.)
