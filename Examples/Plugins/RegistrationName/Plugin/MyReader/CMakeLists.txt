set(classes
  vtkMyPNGReader)

vtk_module_add_module(MyReader
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading                      
  CLASSES ${classes})
