INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
)




ADD_PARAVIEW_PLUGIN(PrismServerPlugin "1.0"
  SERVER_MANAGER_XML PRISMServerManagerXML.xml 
  SERVER_MANAGER_SOURCES vtkPrismFilter.cxx vtkPrismSurfaceReader.cxx vtkPrismCubeAxesActor.cxx vtkSMPrismDoubleRangeDomain.cxx )


