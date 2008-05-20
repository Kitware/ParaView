
# create a paraview plugin containing custom classes that work on the client
# side in the server manager

# in this example, a custom bounds domain is created so that 
# the low and high point of the elevation filter can be initialized
# in the z-direction

INCLUDE(${ParaView_SOURCE_DIR}/VTK/CMake/vtkMakeInstantiator.cmake)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(SM_SOURCES vtkSMMyElevationProxy.cxx vtkSMMyBoundsDomain.cxx)

# need to create methods to let the server manager know how to 
# instantiate the custom classes specified in the server manager xml
VTK_MAKE_INSTANTIATOR3(vtkSMMyProxyInstantiator Instantiator_SRCS
                       "${SM_SOURCES}"
                       VTK_EXPORT
                       "${CMAKE_CURRENT_BINARY_DIR}" "")

# we need server manager wrappings for SM_SOURCES
# put instantiators in GUI_SOURCES because they are client side
# and we need the plugin to be loaded on the client side
ADD_PARAVIEW_PLUGIN(SMMyProxy "1.0"
  SERVER_MANAGER_XML MyProxy.xml
  SERVER_MANAGER_SOURCES ${SM_SOURCES}
  GUI_SOURCES ${Instantiator_SRCS})

