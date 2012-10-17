# Wrap all modules marked for client server wrapping
# PARAVIEW_CURRENT_CS_MODULES is an interal cache entry created
# in SMApplicaton/CMakeLists.txt
if (TARGET vtkWrapClientServer)
  include (vtkClientServerWrapping)
  foreach(_module ${PARAVIEW_CURRENT_CS_MODULES})
    if (NOT ${_module}_EXCLUDE_FROM_WRAPPING)
      set_property(GLOBAL APPEND PROPERTY VTK_CS_WRAPPED ${_module})
      vtk_add_cs_wrapping(${_module})
    endif()
  endforeach()
else()
  message(AUTHOR_WARNING "vtkWrapClientServer could not be found, "
    "no client-server wrappings will be generated.")
endif()
