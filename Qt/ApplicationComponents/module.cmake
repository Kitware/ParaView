vtk_module(pqApplicationComponents
  GROUPS
    ParaViewQt
  DEPENDS
    pqComponents
    vtkGUISupportQt
  PRIVATE_DEPENDS
    vtkjsoncpp
    vtkPVAnimation
    vtkPVServerManagerDefault
    vtkPVServerManagerRendering
    vtksys
  COMPILE_DEPENDS
    # doesn't really depend on this, but a good way to enable this
    # tool when ParaView UI is being built.
    vtkUtilitiesLegacyColorMapXMLToJSON
  EXCLUDE_FROM_WRAPPING
  TEST_LABELS
    PARAVIEW
)
