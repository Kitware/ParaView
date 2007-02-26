
ADD_PARAVIEW_PLUGIN(SMMyElevation "1.0"
  SERVER_MANAGER_XML MyElevationFilter.xml 
  SERVER_MANAGER_SOURCES vtkMyElevationFilter.cxx)

