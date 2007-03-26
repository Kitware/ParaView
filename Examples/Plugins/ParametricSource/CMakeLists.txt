
# create a paraview plugin containing server manager xml and the server
# manager classes to build
# this plugin can be loaded on the server side

ADD_PARAVIEW_PLUGIN(SMParametricSource "1.0"
  SERVER_MANAGER_XML ParametricSource.xml)

