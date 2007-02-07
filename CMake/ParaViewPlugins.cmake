# Requires ParaView_SOURCE_DIR and ParaView_BINARY_DIR to be set.

# create a plugin given a name for the module, an XML file and
# source files of VTK objects to wrap
MACRO(ADD_PARAVIEW_SM_PLUGIN ModuleName XMLFile)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
  GET_FILENAME_COMPONENT(XML_FILE "${XMLFile}" ABSOLUTE)
  GET_FILENAME_COMPONENT(XML_NAME "${XMLFile}" NAME_WE)
  SET(XML_HEADER "${CMAKE_CURRENT_BINARY_DIR}/vtkSMXML.h")
  SET(MODULE_NAME ${ModuleName})
  
  SET(XML_IFACE_PREFIX ${ModuleName})
  SET(XML_IFACE_SUFFIX Interface)
  SET(XML_IFACE_GET_METHOD GetInterfaces)
  SET(XML_GET_INTERFACE ${XML_IFACE_PREFIX}${XML_NAME}${XML_IFACE_GET_METHOD})
  
  ADD_CUSTOM_COMMAND(
    OUTPUT "${XML_HEADER}"
    DEPENDS "${XML_FILE}" "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
    COMMAND "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
    ARGS "${XML_HEADER}"
    ${XML_IFACE_PREFIX} ${XML_IFACE_SUFFIX} ${XML_IFACE_GET_METHOD}
    "${XML_FILE}"
    )
  CONFIGURE_FILE("${ParaView_SOURCE_DIR}/PluginInit.cxx.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/PluginInit.cxx" @ONLY)
  SET(HDRS)
  FOREACH(SRC ${ARGN})
    GET_FILENAME_COMPONENT(src_name "${SRC}" NAME_WE)
    GET_FILENAME_COMPONENT(src_path "${SRC}" ABSOLUTE)
    GET_FILENAME_COMPONENT(src_path "${src_path}" PATH)
    SET(HDRS ${HDRS} "${src_path}/${src_name}.h")
  ENDFOREACH(SRC ${ARGN})

  VTK_WRAP_ClientServer(${ModuleName} CS_SRCS ${HDRS})
  
  ADD_LIBRARY(${ModuleName} SHARED ${ARGN} ${CS_SRCS}
    ${XML_HEADER}
    ${CMAKE_CURRENT_BINARY_DIR}/PluginInit.cxx
    )
  TARGET_LINK_LIBRARIES(${ModuleName} vtkPVFiltersCS)

ENDMACRO(ADD_PARAVIEW_SM_PLUGIN)

# create implementation for a custom panel interface
MACRO(ADD_PANEL_INTERFACE OUTSRCS ClassName XMLName XMLGroup)
  SET(PANEL_NAME ${ClassName})
  SET(PANEL_XML_NAME ${XMLName})
  SET(PANEL_XML_GROUP ${XMLGroup})

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqObjectPanelImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqObjectPanelImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx @ONLY)
  
  QT4_WRAP_CPP(IFACE_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h)

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h
      ${IFACE_MOC_SRCS}
      )

ENDMACRO(ADD_PANEL_INTERFACE)

# create implementation for a Qt/ParaView plugin given a 
# module name and a list of interfaces
MACRO(ADD_GUI_PLUGIN OUTSRCS NAME)
  SET(INTERFACE_INCLUDES)
  SET(INTERFACE_INSTANCES)
  SET(PLUGIN_NAME ${NAME})

  FOREACH(IFACE ${ARGN})
    SET(TMP "#include \"${IFACE}Implementation.h\"")
    SET(INTERFACE_INCLUDES "${INTERFACE_INCLUDES}\n${TMP}")
    SET(TMP "  this->Interfaces.append(new ${IFACE}Implementation(this));")
    SET(INTERFACE_INSTANCES "${INTERFACE_INSTANCES}\n${TMP}")
  ENDFOREACH(IFACE ${ARGN})
  
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Core/pqPluginImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}PluginImplementation.cxx @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Core/pqPluginImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}PluginImplementation.h @ONLY)
  
  QT4_WRAP_CPP(PLUGIN_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}PluginImplementation.h)
  
  SET(${OUTSRCS} ${PLUGIN_MOC_SRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}PluginImplementation.cxx)

ENDMACRO(ADD_GUI_PLUGIN)

