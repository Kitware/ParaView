set (__depends)
if (PARAVIEW_ENABLE_PYTHON)
  set (__depends pqPython)
endif()

vtk_module(pqComponents
  GROUPS
    ParaViewQt
  DEPENDS
    pqCore
    ${__depends}
  EXCLUDE_FROM_WRAPPING
)
