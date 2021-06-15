if (NOT BUILD_SHARED_LIBS)
  message(AUTHOR_WARNING
    "Compiling the Catalyst implementation as shared due to a lack of static "
    "build support in Catalyst itself..")
  set(BUILD_SHARED_LIBS ON)
endif ()

catalyst_implementation(
  NAME    paraview
  TARGET  catalyst-paraview
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

_vtk_module_install(catalyst-paraview)
_vtk_module_apply_properties(catalyst-paraview)
