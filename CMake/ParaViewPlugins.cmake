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

# create implementation for a custom object panel interface
# ADD_PARAVIEW_OBJECT_PANEL(
#    OUTIFACES
#    OUTSRCS
#    [CLASS_NAME classname]
#    XML_NAME xmlname
#    XML_GROUP xmlgroup
#  CLASS_NAME: optional name for the class that implements pqObjectPanel
#              if none give ${XML_NAME}Panel is assumed
#  XML_NAME : the xml name of the source/filter this panel corresponds with
#  XML_GROUP : the xml group of the source/filter this panel corresponds with
MACRO(ADD_PARAVIEW_OBJECT_PANEL OUTIFACES OUTSRCS)

  SET(ARG_CLASS_NAME)
  
  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;XML_NAME;XML_GROUP" "" ${ARGN} )

  IF(ARG_CLASS_NAME)
    SET(PANEL_NAME ${ARG_CLASS_NAME})
  ELSE(ARG_CLASS_NAME)
    SET(PANEL_NAME ${ARG_XML_NAME}Panel)
  ENDIF(ARG_CLASS_NAME)
  SET(PANEL_XML_NAME ${ARG_XML_NAME})
  SET(PANEL_XML_GROUP ${ARG_XML_GROUP})
  SET(${OUTIFACES} ${PANEL_NAME})

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqObjectPanelImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqObjectPanelImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  QT4_WRAP_CPP(PANEL_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

 SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h
      ${PANEL_MOC_SRCS}
      )

ENDMACRO(ADD_PARAVIEW_OBJECT_PANEL)

# create implementation for a custom display panel interface
# ADD_PARAVIEW_DISPLAY_PANEL(
#    OUTIFACES
#    OUTSRCS
#    CLASS_NAME classname
#    XML_NAME xmlname
#  CLASS_NAME: pqDisplayPanel
#  XML_NAME : the xml name of the display this panel corresponds with
MACRO(ADD_PARAVIEW_DISPLAY_PANEL OUTIFACES OUTSRCS)
  
  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;XML_NAME" "" ${ARGN} )
  
  SET(PANEL_NAME ${ARG_CLASS_NAME})
  SET(PANEL_XML_NAME ${ARG_XML_NAME})
  SET(${OUTIFACES} ${PANEL_NAME})

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqDisplayPanelImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqDisplayPanelImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  QT4_WRAP_CPP(DISPLAY_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${PANEL_NAME}Implementation.h
      ${DISPLAY_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_DISPLAY_PANEL)

# create implementation for a custom view 
# Usage:
# ADD_PARAVIEW_VIEW_MODULE( OUTIFACES OUTSRCS
#     VIEW_TYPE Type
#     VIEW_XML_GROUP Group
#     [VIEW_NAME Name]
#     [DISPLAY_PANEL Display]
#     [DISPLAY_TYPE Display]

# for the given server manager XML
#  <SourceProxy name="MyFilter" class="MyFilter" label="My Filter">
#    ...
#    <Hints>
#      <View type="MyView" />
#    </Hints>
#  </SourceProxy>
#  ....
# <ProxyGroup name="plotmodules">
#  <ViewModuleProxy name="MyViewViewModule"
#      base_proxygroup="rendermodules" base_proxyname="ViewModule"
#      display_name="MyDisplay">
#  </ViewModuleProxy>
# </ProxyGroup>

#  VIEW_TYPE = "MyView"
#  VIEW_XML_GROUP = "plotmodules"
#  VIEW_NAME is optional and gives a friendly name for the view type
#  DISPLAY_TYPE is optional and defaults to pqConsumerDisplay
#  DISPLAY_PANEL gives the name of the display panel
#  DISPLAY_XML is the XML name of the display for this view and is required if
#     DISPLAY_PANEL is set
#
#  if DISPLAY_PANEL is MyDisplay, then "MyDisplayPanel.h" is looked for.
#  a class MyView derived from pqGenericViewModule is expected to be in "MyView.h"

MACRO(ADD_PARAVIEW_VIEW_MODULE OUTIFACES OUTSRCS)
  
  SET(PANEL_SRCS)
  SET(ARG_VIEW_TYPE)
  SET(ARG_VIEW_NAME)
  SET(ARG_VIEW_XML_GROUP)
  SET(ARG_DISPLAY_PANEL)
  SET(ARG_DISPLAY_XML)
  SET(ARG_DISPLAY_TYPE)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "VIEW_TYPE;VIEW_XML_GROUP;VIEW_NAME;DISPLAY_PANEL;DISPLAY_TYPE;DISPLAY_XML"
                  "" ${ARGN} )

  IF(NOT ARG_VIEW_TYPE OR NOT ARG_VIEW_XML_GROUP)
    MESSAGE(ERROR "ADD_PARAVIEW_VIEW_MODULE called without VIEW_TYPE or VIEW_XML_GROUP")
  ENDIF(NOT ARG_VIEW_TYPE OR NOT ARG_VIEW_XML_GROUP)

  IF(ARG_DISPLAY_PANEL)
    IF(NOT ARG_DISPLAY_XML)
      MESSAGE(ERROR "ADD_PARAVIEW_VIEW_MODULE called with DISPLAY_PANEL but DISPLAY_XML not specified")
    ENDIF(NOT ARG_DISPLAY_XML)
  ENDIF(ARG_DISPLAY_PANEL)


  SET(${OUTIFACES} ${ARG_VIEW_TYPE})
  SET(VIEW_TYPE ${ARG_VIEW_TYPE})
  SET(VIEW_XML_GROUP ${ARG_VIEW_XML_GROUP})
  IF(ARG_VIEW_NAME)
    SET(VIEW_TYPE_NAME ${ARG_VIEW_NAME})
  ELSE(ARG_VIEW_NAME)
    SET(VIEW_TYPE_NAME ${ARG_VIEW_TYPE})
  ENDIF(ARG_VIEW_NAME)

  IF(ARG_DISPLAY_TYPE)
    SET(DISPLAY_TYPE ${ARG_DISPLAY_TYPE})
  ELSE(ARG_DISPLAY_TYPE)
    SET(DISPLAY_TYPE "pqConsumerDisplay")
  ENDIF(ARG_DISPLAY_TYPE)

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Core/pqViewModuleImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${VIEW_TYPE}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Core/pqViewModuleImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${VIEW_TYPE}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  QT4_WRAP_CPP(VIEW_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${VIEW_TYPE}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  IF(ARG_DISPLAY_PANEL)
    ADD_PARAVIEW_DISPLAY_PANEL(OUT_PANEL_IFACES PANEL_SRCS 
                               CLASS_NAME ${ARG_DISPLAY_PANEL} 
                               XML_NAME ${ARG_DISPLAY_XML})
    SET(${OUTIFACES} ${ARG_VIEW_TYPE} ${OUT_PANEL_IFACES})
  ENDIF(ARG_DISPLAY_PANEL)

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${VIEW_TYPE}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${VIEW_TYPE}Implementation.h
      ${VIEW_MOC_SRCS}
      ${PANEL_SRCS}
      )

ENDMACRO(ADD_PARAVIEW_VIEW_MODULE)


# create implementation for a custom display panel interface
# ADD_PARAVIEW_ACTION_GROUP(
#    OUTIFACES
#    OUTSRCS
#    CLASS_NAME classname
#    GROUP_NAME groupName
#
#    CLASS_NAME is the name of the class that implements a QActionGroup
#    GROUP_NAME is the name of the group "MenuBar/MyMenu" or "ToolBar/MyToolBar"
MACRO(ADD_PARAVIEW_ACTION_GROUP OUTIFACES OUTSRCS)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;GROUP_NAME" "" ${ARGN} )
  
  SET(CLASS_NAME ${ARG_CLASS_NAME})
  SET(GROUP_NAME ${ARG_GROUP_NAME})
  SET(${OUTIFACES} ${CLASS_NAME})

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqActionGroupImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqActionGroupImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${CLASS_NAME}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${CLASS_NAME}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_ACTION_GROUP)

# create implementation for a Qt/ParaView plugin given a 
# module name and a list of interfaces
# ADD_PARAVIEW_GUI_EXTENSION(OUTSRCS NAME INTERFACES iface1;iface2;iface3)
MACRO(ADD_PARAVIEW_GUI_EXTENSION OUTSRCS NAME)
  
  SET(INTERFACE_INCLUDES)
  SET(INTERFACE_INSTANCES)
  SET(PLUGIN_NAME ${NAME})
  SET(ARG_INTERFACES)
  
  PV_PLUGIN_PARSE_ARGUMENTS(ARG "INTERFACES" "" ${ARGN} )

  IF(ARG_INTERFACES)
    FOREACH(IFACE ${ARG_INTERFACES})
      SET(TMP "#include \"${IFACE}Implementation.h\"")
      SET(INTERFACE_INCLUDES "${INTERFACE_INCLUDES}\n${TMP}")
      SET(TMP "  this->Interfaces.append(new ${IFACE}Implementation(this));")
      SET(INTERFACE_INSTANCES "${INTERFACE_INSTANCES}\n${TMP}")
    ENDFOREACH(IFACE ${ARG_INTERFACES})
  ELSE(ARG_INTERFACES)
    SET(INTERFACE_INCLUDES)
    SET(INTERFACE_INSTANCES)
  ENDIF(ARG_INTERFACES)
  
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

ENDMACRO(ADD_PARAVIEW_GUI_EXTENSION)

# create a plugin from sources
# ADD_PARAVIEW_PLUGIN(Name Version
#     [SERVER_MANAGER_SOURCES source files]
#     [SERVER_MANAGER_XML XMLFile]
#     [GUI_INTERFACES interface1 interface2]
#     [GUI_RESOURCES qrc1 qrc2]
#     [SOURCES source files]
#  )
MACRO(ADD_PARAVIEW_PLUGIN NAME VERSION)

  IF(PARAVIEW_BUILD_SHARED_LIBS)

    SET(GUI_SRCS)
    SET(SM_SRCS)
    SET(ARG_GUI_INTERFACES)
    SET(ARG_SERVER_MANAGER_SOURCES)
    
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
    
    PV_PLUGIN_PARSE_ARGUMENTS(ARG "SERVER_MANAGER_SOURCES;SERVER_MANAGER_XML;GUI_INTERFACES;GUI_RESOURCES;SOURCES"
                    "" ${ARGN} )

    IF(ARG_SERVER_MANAGER_SOURCES OR ARG_SERVER_MANAGER_XML)
      ADD_SERVER_MANAGER_EXTENSION(SM_SRCS ${NAME} "${ARG_SERVER_MANAGER_XML}"
                                   ${ARG_SERVER_MANAGER_SOURCES})
    ENDIF(ARG_SERVER_MANAGER_SOURCES OR ARG_SERVER_MANAGER_XML)

    IF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES)
      ADD_PARAVIEW_GUI_EXTENSION(GUI_SRCS ${NAME} INTERFACES "${ARG_GUI_INTERFACES}")
    ENDIF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES)

    IF(ARG_GUI_RESOURCES)
      QT4_ADD_RESOURCES(QT_RCS ${ARG_GUI_RESOURCES})
    ENDIF(ARG_GUI_RESOURCES)

    ADD_LIBRARY(${NAME} SHARED ${GUI_SRCS} ${QT_RCS} ${SM_SRCS} ${ARG_SOURCES}
                               ${ARG_SERVER_MANAGER_SOURCES})
    IF(GUI_SRCS)
      TARGET_LINK_LIBRARIES(${NAME} pqComponents)
    ENDIF(GUI_SRCS)
    IF(SM_SRCS)
      TARGET_LINK_LIBRARIES(${NAME} vtkPVFiltersCS)
    ENDIF(SM_SRCS)
  
  ELSE(PARAVIEW_BUILD_SHARED_LIBS)
    MESSAGE(STATUS "ParaView plugins are disabled.  Please build ParaView with shared libraries.") 
  ENDIF(PARAVIEW_BUILD_SHARED_LIBS)

ENDMACRO(ADD_PARAVIEW_PLUGIN)

