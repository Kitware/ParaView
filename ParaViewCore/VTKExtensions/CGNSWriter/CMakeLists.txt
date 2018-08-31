# -----------------------------------------------------------------------------
# Disable some warnings
# -----------------------------------------------------------------------------
if (WIN32)
  if (MSVC_VERSION GREATER 1400)
    # -------------------------------------------------------------------------
    # Disable deprecation warnings for standard C and STL functions in VS2005
    # and newer
    # -------------------------------------------------------------------------
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
  endif ()
endif()

set(Module_SRCS
  vtkCGNSWriter.cxx  
  )

vtk_module_library(vtkPVVTKExtensionsCGNSWriter ${Module_SRCS})
