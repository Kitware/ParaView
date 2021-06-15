if (NOT BUILD_SHARED_LIBS)
  message(AUTHOR_WARNING
    "Compiling the Catalyst implementation as shared due to a lack of static "
    "build support in Catalyst itself..")
  set(BUILD_SHARED_LIBS ON)
endif ()

if (WIN32)
  set(catalyst_library_destination
    "${_vtk_build_RUNTIME_DESTINATION}")
else ()
  set(catalyst_library_destination
    "${_vtk_build_LIBRARY_DESTINATION}/catalyst")
  file(RELATIVE_PATH _catalyst_impl_relpath
    "/prefix/${catalyst_library_destination}"
    "/prefix/${_vtk_build_LIBRARY_DESTINATION}")
  if (APPLE)
    set(catalyst_rel_rpath
      "@loader_path/${_catalyst_impl_relpath}")
  else ()
    set(catalyst_rel_rpath
      "$ORIGIN/${_catalyst_impl_relpath}")
  endif ()

  list(APPEND CMAKE_INSTALL_RPATH
    "${catalyst_rel_rpath}")
endif ()

catalyst_implementation(
  NAME    paraview
  TARGET  catalyst-paraview
  LIBRARY_DESTINATION
    "${catalyst_library_destination}"
  SOURCES
    ParaViewCatalyst.cxx
    vtkCatalystBlueprint.cxx
    vtkCatalystBlueprint.h)

target_link_libraries(catalyst-paraview
  PRIVATE
    ParaView::InSitu
    ParaView::VTKExtensionsCore
    ParaView::VTKExtensionsConduit
    ParaView::RemotingServerManager)

if (TARGET VTK::ParallelMPI)
  target_link_libraries(catalyst-paraview
    PRIVATE
      VTK::ParallelMPI)
endif()
target_compile_definitions(catalyst-paraview
  PRIVATE
    "VTK_MODULE_ENABLE_VTK_ParallelMPI=$<TARGET_EXISTS:VTK::ParallelMPI>")

if (TARGET VTK::WrappingPythonCore AND VTK_PYTHON_OPTIONAL_LINK)
  # if Python is enabled and optionally linking with Python,
  # we need to link against Python non-optionally here.
  find_package(Python3 COMPONENTS Development)
  target_link_libraries(catalyst-paraview
    PRIVATE
      Python3::Python)
endif()

# Clear the `-pvVERSION` suffix (if any).
set(_vtk_build_LIBRARY_NAME_SUFFIX "")
# Clear version information.
set(_vtk_build_VERSION "")
set(_vtk_build_SOVERSION "")

_vtk_module_install(catalyst-paraview)
_vtk_module_apply_properties(catalyst-paraview)
