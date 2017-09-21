set (__dependencies)
if (PARAVIEW_USE_OSPRAY)
  list (APPEND __dependencies vtkRenderingOSPRay)
endif ()

vtk_module(vtkPVServerManagerRendering
  GROUPS
    ParaViewRendering
  IMPLEMENTS
    vtkPVServerManagerCore
  DEPENDS
    vtkPVServerImplementationRendering
    vtkPVServerManagerCore
    vtkjsoncpp
    ${__dependencies}
  PRIVATE_DEPENDS
    vtkCommonColor
    vtksys
  COMPILE_DEPENDS
    vtkUtilitiesProcessXML
  TEST_DEPENDS
    vtkPVServerManagerApplication
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVServerManager
)
