# A little extra info on the directory structure in the source tree.
# <CURRENT_SOURCE_DIR>
#     VTK module files and CMakeLists.txt files.
# <CURRENT_SOURCE_DIR>/vtkpygments
#     Directory corresponding to the Pygments source from pygments.org
#     stripped down to remove files we do not care about.
# <CURRENT_SOURCE_DIR>/vtkpygments/pygments
#     The pygments Python package which gets imported in Python interpretor.
include(vtkModuleMacrosPython)
vtk_module_python_package(${vtk-module} "vtkpygments/pygments" CAN_USE_SYSTEM)
