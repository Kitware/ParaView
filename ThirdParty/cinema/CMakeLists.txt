# A little extra info on the directory structure in the source tree.
# <CURRENT_SOURCE_DIR>
#     VTK module files and CMakeLists.txt files.
# <CURRENT_SOURCE_DIR>/vtkcinemapython
#     Directory corresponding to the "cinema_python" source from
#     https://gitlab.kitware.com/cinema/cinema_python
# <CURRENT_SOURCE_DIR>/vtkcinemapython/cinema_python
#     The "cinema_python" Python package which gets imported in Python interpretor.
include(vtkModuleMacrosPython)
vtk_module_python_package(${vtk-module} "vtkcinemapython/cinema_python" CAN_USE_SYSTEM)
