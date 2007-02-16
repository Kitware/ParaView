# Requires ParaView_SOURCE_DIR and ParaView_BINARY_DIR to be set.

# helper PV_PLUGIN_LIST_CONTAINS macro
MACRO(PV_PLUGIN_LIST_CONTAINS var value)
  SET(${var})
  FOREACH (value2 ${ARGN})
    IF (${value} STREQUAL ${value2})
      SET(${var} TRUE)
    ENDIF (${value} STREQUAL ${value2})
  ENDFOREACH (value2)
ENDMACRO(PV_PLUGIN_LIST_CONTAINS)


# helper PV_PLUGIN_PARSE_ARGUMENTS macro
MACRO(PV_PLUGIN_PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)
  
  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})
    PV_PLUGIN_LIST_CONTAINS(is_arg_name ${arg} ${arg_names})
    IF (is_arg_name)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name)
      PV_PLUGIN_LIST_CONTAINS(is_option ${arg} ${option_names})
      IF (is_option)
        SET(${prefix}_${arg} TRUE)
      ELSE (is_option)
        SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option)
    ENDIF (is_arg_name)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PV_PLUGIN_PARSE_ARGUMENTS)

# create plugin glue code for a server manager extension
# consisting of server manager XML and VTK classes
# sets OUTSRCS with the generated code
MACRO(ADD_SERVER_MANAGER_EXTENSION OUTSRCS Name XMLFile)
    
  GET_FILENAME_COMPONENT(XML_NAME "${XMLFile}" NAME_WE)

  IF(XML_NAME)
    GET_FILENAME_COMPONENT(XML_FILE "${XMLFile}" ABSOLUTE)
    SET(XML_HEADER "${CMAKE_CURRENT_BINARY_DIR}/vtkSMXML.h")
    SET(MODULE_NAME ${Name})
    
    SET(XML_IFACE_PREFIX ${Name})
    SET(XML_IFACE_SUFFIX Interface)
    SET(XML_IFACE_GET_METHOD GetInterfaces)
    SET(XML_GET_INTERFACE ${XML_IFACE_PREFIX}${XML_NAME}${XML_IFACE_GET_METHOD})
    SET(HAVE_XML 1)
    
    ADD_CUSTOM_COMMAND(
      OUTPUT "${XML_HEADER}"
      DEPENDS "${XML_FILE}" "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
      COMMAND "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
      ARGS "${XML_HEADER}"
      ${XML_IFACE_PREFIX} ${XML_IFACE_SUFFIX} ${XML_IFACE_GET_METHOD}
      "${XML_FILE}"
      )
  ELSE(XML_NAME)
    SET(XML_HEADER)
    SET(HAVE_XML 0)
  ENDIF(XML_NAME)
  
  SET(HDRS)
  FOREACH(SRC ${ARGN})
    GET_FILENAME_COMPONENT(src_name "${SRC}" NAME_WE)
    GET_FILENAME_COMPONENT(src_path "${SRC}" ABSOLUTE)
    GET_FILENAME_COMPONENT(src_path "${src_path}" PATH)
    SET(HDRS ${HDRS} "${src_path}/${src_name}.h")
  ENDFOREACH(SRC ${ARGN})
  
  IF(HDRS)
    VTK_WRAP_ClientServer(${Name} CS_SRCS ${HDRS})
    SET(HAVE_SRCS 1)
  ELSE(HDRS)
    SET(HAVE_SRCS 0)
  ENDIF(HDRS)
  
  CONFIGURE_FILE("${ParaView_SOURCE_DIR}/Servers/ServerManager/vtkSMPluginInit.cxx.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/vtkSMPluginInit.cxx" @ONLY)

  SET(${OUTSRCS} ${CS_SRCS} ${XML_HEADER}
    ${CMAKE_CURRENT_BINARY_DIR}/vtkSMPluginInit.cxx
    )
  
ENDMACRO(ADD_SERVER_MANAGER_EXTENSION)

# create implementation for a custom panel interface
MACRO(ADD_GUI_PANEL_INTERFACE OUTSRCS ClassName XMLName XMLGroup)
  SET(PANEL_NAME ${ClassName})
  SET(PANEL_XML_NAME ${XMLName})
  SET(PANEL_XML_GROUP ${XMLGroup})

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqObjectPanelImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqObjectPanelImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  QT4_WRAP_CPP(IFACE_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h
      ${IFACE_MOC_SRCS}
      )

ENDMACRO(ADD_GUI_PANEL_INTERFACE)

# create implementation for a custom panel interface
MACRO(ADD_VIEW_MODULE_INTERFACE OUTSRCS ClassName ViewName DisplayType
    ViewXMLType ViewXMLGroup ViewXMLName)
  SET(VIEW_TYPE ${ClassName})
  SET(VIEW_TYPE_NAME ${ViewName})
  IF(${DisplayType})
    SET(DISPLAY_TYPE ${ViewName})
  ELSE(${DisplayType})
    SET(DISPLAY_TYPE "pqConsumerDisplay")
  ENDIF(${DisplayType})
  SET(VIEW_XML_TYPE ${ViewXMLType})
  SET(VIEW_XML_GROUP ${ViewXMLGroup})
  SET(VIEW_XML_NAME ${ViewXMLName})

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Core/pqViewModuleImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${VIEW_TYPE}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Core/pqViewModuleImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${VIEW_TYPE}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  QT4_WRAP_CPP(IFACE_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${VIEW_TYPE}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${VIEW_TYPE}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${VIEW_TYPE}Implementation.h
      ${IFACE_MOC_SRCS}
      )

ENDMACRO(ADD_VIEW_MODULE_INTERFACE)

# create implementation for a Qt/ParaView plugin given a 
# module name and a list of interfaces
MACRO(ADD_GUI_EXTENSION OUTSRCS NAME)
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
  
  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  QT4_WRAP_CPP(PLUGIN_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}PluginImplementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")
  
  SET(${OUTSRCS} ${PLUGIN_MOC_SRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}PluginImplementation.cxx)

ENDMACRO(ADD_GUI_EXTENSION)

# create a plugin from sources
# ADD_PARAVIEW_PLUGIN(Name Version
#     [SERVER_MANAGER_SOURCES source files]
#     [SERVER_MANAGER_XML XMLFile]
#     [GUI_INTERFACES interface1 interface2]
#     [SOURCES source files]
#  )
MACRO(ADD_PARAVIEW_PLUGIN NAME VERSION)

  SET(GUI_SRCS)
  SET(SM_SRCS)
  SET(ARG_GUI_INTERFACES)
  SET(ARG_SERVER_MANAGER_SOURCES)
  
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
  
  PV_PLUGIN_PARSE_ARGUMENTS(ARG "SERVER_MANAGER_SOURCES;SERVER_MANAGER_XML;GUI_INTERFACES;SOURCES"
                  "" ${ARGN} )

  IF(ARG_SERVER_MANAGER_SOURCES OR ARG_SERVER_MANAGER_XML)
    ADD_SERVER_MANAGER_EXTENSION(SM_SRCS ${NAME} "${ARG_SERVER_MANAGER_XML}"
                                 ${ARG_SERVER_MANAGER_SOURCES})
  ENDIF(ARG_SERVER_MANAGER_SOURCES OR ARG_SERVER_MANAGER_XML)

  IF(ARG_GUI_INTERFACES)
    ADD_GUI_EXTENSION(GUI_SRCS ${NAME} ${ARG_GUI_INTERFACES})
  ENDIF(ARG_GUI_INTERFACES)

  ADD_LIBRARY(${NAME} SHARED ${GUI_SRCS} ${SM_SRCS} ${ARG_SOURCES})
  IF(GUI_SRCS)
    TARGET_LINK_LIBRARIES(${NAME} pqComponents)
  ENDIF(GUI_SRCS)
  IF(SM_SRCS)
    TARGET_LINK_LIBRARIES(${NAME} vtkPVFiltersCS)
  ENDIF(SM_SRCS)

ENDMACRO(ADD_PARAVIEW_PLUGIN)

