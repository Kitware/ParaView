paraview_add_plugin(ComplexModuleArchitecture
  VERSION "1.0"
  SERVER_MANAGER_XML filters.xml
  MODULES
    ComplexPluginArchitecture::Shared
    ComplexModuleArchitecture::Core 
    ComplexModuleArchitecture::Filters
  MODULE_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/../Core/vtk.module"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Filters/vtk.module")

target_link_libraries(ComplexModuleArchitecture 
        PRIVATE 
          ComplexPluginArchitecture::Shared 
          ComplexModuleArchitecture::Core 
          ComplexModuleArchitecture::Filters)
