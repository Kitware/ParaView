set(PVW_DEPENDS
    vtkWebCore
    vtkWebPython
    vtkWebGLExporter
    vtkParaViewWebCore
    vtkParaViewWebPython
)

if(VTK_PYTHON_VERSION EQUAL 2)
    list(APPEND PVW_DEPENDS vtkWebPython2 vtkParaViewWebPython2)
endif()

vtk_module(vtkParaViewWeb
  DEPENDS
    ${PVW_DEPENDS}
  EXCLUDE_FROM_WRAPPING
  TEST_LABELS
    PARAVIEW
)
