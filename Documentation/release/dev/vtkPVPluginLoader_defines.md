## Fix adding PARAVIEW_PLUGIN_LOADER_PATHS to vtkPVPluginLoader.cxx defines

Adding PARAVIEW_PLUGIN_LOADER_PATHS to compile definitions for vtkPVPluginLoader.cxx
is fixed to stop wiping BUILD_SHARED_LIBS definitions.
