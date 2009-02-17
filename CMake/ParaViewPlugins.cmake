# Requires ParaView_SOURCE_DIR and ParaView_BINARY_DIR to be set.

INCLUDE("${VTK_MAKE_INSTANTIATOR}/vtkMakeInstantiator.cmake")

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

# Macro to generate a header xml given an XML.
MACRO(GENERATE_SERVER_MANAGER_XML_HEADER OUT_XML_HEADER Name XMLFile)
  IF(PARAVIEW_PROCESS_XML_EXECUTABLE)
    FOREACH(TMPXML ${XMLFile})
      GET_FILENAME_COMPONENT(TMP_XML_FILE "${TMPXML}" ABSOLUTE)
      GET_FILENAME_COMPONENT(XML_NAME "${TMPXML}" NAME_WE)
      SET(XML_FILES ${XML_FILES} ${TMP_XML_FILE})
      SET(HAVE_XML 1)
    ENDFOREACH(TMPXML)

    IF(HAVE_XML)
      SET(XML_HEADER "${CMAKE_CURRENT_BINARY_DIR}/vtkSMXML_${Name}.h")

      ADD_CUSTOM_COMMAND(
        OUTPUT "${XML_HEADER}"
        DEPENDS ${XML_FILES} "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
        COMMAND "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
        ARGS "${XML_HEADER}" "vtkSM" "XML" "GetString" ${XML_FILES}
        )

      SET (${OUT_XML_HEADER} ${XML_HEADER})
    ENDIF(HAVE_XML)

  ELSE(PARAVIEW_PROCESS_XML_EXECUTABLE)
    MESSAGE("kwProcessXML not found.  Plugin may not build correctly")
  ENDIF(PARAVIEW_PROCESS_XML_EXECUTABLE)
ENDMACRO(GENERATE_SERVER_MANAGER_XML_HEADER)

# create plugin glue code for a server manager extension
# consisting of server manager XML and VTK classes
# sets OUTSRCS with the generated code
MACRO(ADD_SERVER_MANAGER_EXTENSION OUTSRCS Name XMLFile)
    
  SET(XML_HEADER)
  SET(XML_FILES)
  SET(HAVE_XML 0)
  SET(XML_GET_INTERFACE)
  
  SET(XML_IFACE_PREFIX ${Name})
  SET(XML_IFACE_GET_METHOD GetInterfaces)
  SET(XML_IFACE_SUFFIX Interface)
  SET(MODULE_NAME ${Name})

  IF(PARAVIEW_PROCESS_XML_EXECUTABLE)
    FOREACH(TMPXML ${XMLFile})
      GET_FILENAME_COMPONENT(TMP_XML_FILE "${TMPXML}" ABSOLUTE)
      GET_FILENAME_COMPONENT(XML_NAME "${TMPXML}" NAME_WE)
      SET(XML_FILES ${XML_FILES} ${TMP_XML_FILE})
      IF(XML_GET_INTERFACE)
        SET(XML_GET_INTERFACE "${XML_GET_INTERFACE},\n    ")
      ELSE(XML_GET_INTERFACE)
        SET(XML_GET_INTERFACE "    ")
      ENDIF(XML_GET_INTERFACE)
      SET(XML_GET_INTERFACE "${XML_GET_INTERFACE} ${XML_IFACE_PREFIX}${XML_NAME}${XML_IFACE_GET_METHOD}()")
      SET(HAVE_XML 1)
    ENDFOREACH(TMPXML)

    IF(HAVE_XML)
      SET(XML_HEADER "${CMAKE_CURRENT_BINARY_DIR}/vtkSMXML_${Name}.h")

      ADD_CUSTOM_COMMAND(
        OUTPUT "${XML_HEADER}"
        DEPENDS ${XML_FILES} "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
        COMMAND "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
        ARGS "${XML_HEADER}"
        ${XML_IFACE_PREFIX} ${XML_IFACE_SUFFIX} ${XML_IFACE_GET_METHOD}
        ${XML_FILES}
        )
    ENDIF(HAVE_XML)

  ELSE(PARAVIEW_PROCESS_XML_EXECUTABLE)
    MESSAGE("kwProcessXML not found.  Plugin may not build correctly")
  ENDIF(PARAVIEW_PROCESS_XML_EXECUTABLE)

  SET(HDRS)
  SET(REALSRCS)
  SET(INST_SRCS)

  FOREACH(SRC ${ARGN})
    GET_FILENAME_COMPONENT(src_name "${SRC}" NAME_WE)
    GET_FILENAME_COMPONENT(src_path "${SRC}" ABSOLUTE)
    GET_FILENAME_COMPONENT(src_path "${src_path}" PATH)
    SET(HDR "${src_path}/${src_name}.h")
    SET(HDRS ${HDRS} "${HDR}")
    IF(NOT HDR MATCHES ${SRC})
      SET(REALSRCS ${REALSRCS} ${SRC})
    ENDIF(NOT HDR MATCHES ${SRC})
  ENDFOREACH(SRC ${ARGN})
  
  SET(CS_SRCS)
  IF(HDRS)
    VTK_WRAP_ClientServer(${Name} CS_SRCS "${HDRS}")
    # only generate the instantiator code for cxx classes that'll be included in
    # the plugin
    IF(REALSRCS)
      VTK_MAKE_INSTANTIATOR3(vtkSM${Name}Instantiator INST_SRCS "${REALSRCS}"
        VTK_EXPORT "${CMAKE_CURRENT_BINARY_DIR}" "")
    ENDIF(REALSRCS)
    SET(HAVE_SRCS 1)
  ELSE(HDRS)
    SET(HAVE_SRCS 0)
  ENDIF(HDRS)


  CONFIGURE_FILE(
    "${ParaView_SOURCE_DIR}/Servers/Common/vtkPVPluginInit.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/vtkPVPluginInit_${Name}.cxx" @ONLY)

  SET(${OUTSRCS} ${CS_SRCS} ${INST_SRCS} ${XML_HEADER}
    ${CMAKE_CURRENT_BINARY_DIR}/vtkPVPluginInit_${Name}.cxx
    )
  
ENDMACRO(ADD_SERVER_MANAGER_EXTENSION)

MACRO(ADD_PYTHON_EXTENSION OUTSRCS NAME)
  SET(PYSRCFILES ${ARGN})
  SET(WRAP_PY_HEADERS)
  SET(WRAP_PYTHON_INCLUDES)
  SET(PY_MODULE_LIST)
  SET(PY_LOADER_LIST)
  SET(PY_PACKAGE_FLAGS)
  IF (PARAVIEW_PROCESS_XML_EXECUTABLE)
    SET(QT_COMPONENTS_GUI_RESOURCES_CONTENTS)
    FOREACH(PYFILE ${PYSRCFILES})
      GET_FILENAME_COMPONENT(PYFILE_ABSOLUTE "${PYFILE}" ABSOLUTE)
      GET_FILENAME_COMPONENT(PYFILE_PACKAGE "${PYFILE}" PATH)
      GET_FILENAME_COMPONENT(PYFILE_NAME "${PYFILE}" NAME_WE)
      SET(PACKAGE_FLAG "0")
      IF (PYFILE_PACKAGE)
        STRING(REPLACE "/" "." PYFILE_PACKAGE "${PYFILE_PACKAGE}")
        IF (${PYFILE_NAME} STREQUAL "__init__")
          SET(PYFILE_MODULE "${PYFILE_PACKAGE}")
          SET(PACKAGE_FLAG "1")
        ELSE (${PYFILE_NAME} STREQUAL "__init__")
          SET(PYFILE_MODULE "${PYFILE_PACKAGE}.${PYFILE_NAME}")
        ENDIF (${PYFILE_NAME} STREQUAL "__init__")
      ELSE (PYFILE_PACKAGE)
        SET(PYFILE_MODULE "${PYFILE_NAME}")
      ENDIF (PYFILE_PACKAGE)
      STRING(REPLACE "." "_" PYFILE_MODULE_MANGLED "${PYFILE_MODULE}")
      SET(PY_HEADER "${CMAKE_CURRENT_BINARY_DIR}/WrappedPython_${NAME}_${PYFILE_MODULE_MANGLED}.h")
      ADD_CUSTOM_COMMAND(
        OUTPUT "${PY_HEADER}"
        DEPENDS "${PYFILE_ABSOLUTE}" "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
        COMMAND "${PARAVIEW_PROCESS_XML_EXECUTABLE}"
        ARGS "${PY_HEADER}" "module_${PYFILE_MODULE_MANGLED}_" "_string" "_source" "${PYFILE_ABSOLUTE}"
        )
      SET(WRAP_PY_HEADERS ${WRAP_PY_HEADERS} "${PY_HEADER}")
      SET(WRAP_PYTHON_INCLUDES
        "${WRAP_PYTHON_INCLUDES}#include \"${PY_HEADER}\"\n")
      IF(PY_MODULE_LIST)
        SET(PY_MODULE_LIST
          "${PY_MODULE_LIST},\n        \"${PYFILE_MODULE}\"")
        SET(PY_LOADER_LIST
          "${PY_LOADER_LIST},\n        module_${PYFILE_MODULE_MANGLED}_${PYFILE_NAME}_source()")
        SET(PY_PACKAGE_FLAGS "${PY_PACKAGE_FLAGS}, ${PACKAGE_FLAG}")        
      ELSE(PY_MODULE_LIST)
        SET(PY_MODULE_LIST "\"${PYFILE_MODULE}\"")
        SET(PY_LOADER_LIST
          "module_${PYFILE_MODULE_MANGLED}_${PYFILE_NAME}_source()")
        SET(PY_PACKAGE_FLAGS "${PACKAGE_FLAG}")
      ENDIF(PY_MODULE_LIST)
    ENDFOREACH(PYFILE ${PYSRCFILES})

    # Create source code to get Python source from the plugin.
    SET(PY_INIT_SRC
      "${CMAKE_CURRENT_BINARY_DIR}/vtkPVPythonPluginInit_${NAME}.cxx")
    CONFIGURE_FILE(
      "${ParaView_SOURCE_DIR}/Utilities/VTKPythonWrapping/vtkPVPythonPluginInit.cxx.in"
      "${PY_INIT_SRC}" @ONLY)

    SET(${OUTSRCS} "${PY_INIT_SRC}" "${WRAP_PY_HEADERS}")
  ELSE (PARAVIEW_PROCESS_XML_EXECUTABLE)
    MESSAGE("kwProcessXML not found.  Plugin may not build correctly")
  ENDIF (PARAVIEW_PROCESS_XML_EXECUTABLE)
ENDMACRO(ADD_PYTHON_EXTENSION)

# create implementation for a custom object panel interface
# ADD_PARAVIEW_OBJECT_PANEL(
#    OUTIFACES
#    OUTSRCS
#    [CLASS_NAME classname]
#    XML_NAME xmlname
#    XML_GROUP xmlgroup
#  CLASS_NAME: optional name for the class that implements pqObjectPanel
#              if none give ${XML_NAME} is assumed
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
  SET(PANEL_MOC_SRCS)
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
  SET(DISPLAY_MOC_SRCS)
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
#     [VIEW_XML_NAME Name]
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
#  <ViewProxy name="MyView"
#      base_proxygroup="newviews" base_proxyname="ViewBase"
#      representation_name="MyDisplay">
#  </ViewProxy>
# </ProxyGroup>

#  VIEW_TYPE = "MyView"
#  VIEW_XML_GROUP = "plotmodules"
#  VIEW_XML_NAME is optional and defaults to VIEW_TYPE
#  VIEW_NAME is optional and gives a friendly name for the view type
#  DISPLAY_TYPE is optional and defaults to pqDataRepresentation
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
  SET(ARG_VIEW_XML_NAME)
  SET(ARG_DISPLAY_PANEL)
  SET(ARG_DISPLAY_XML)
  SET(ARG_DISPLAY_TYPE)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "VIEW_TYPE;VIEW_XML_GROUP;VIEW_XML_NAME;VIEW_NAME;DISPLAY_PANEL;DISPLAY_TYPE;DISPLAY_XML"
                  "" ${ARGN} )

  IF(NOT ARG_VIEW_TYPE OR NOT ARG_VIEW_XML_GROUP)
    MESSAGE(ERROR " ADD_PARAVIEW_VIEW_MODULE called without VIEW_TYPE or VIEW_XML_GROUP")
  ENDIF(NOT ARG_VIEW_TYPE OR NOT ARG_VIEW_XML_GROUP)

  IF(ARG_DISPLAY_PANEL)
    IF(NOT ARG_DISPLAY_XML)
      MESSAGE(ERROR " ADD_PARAVIEW_VIEW_MODULE called with DISPLAY_PANEL but DISPLAY_XML not specified")
    ENDIF(NOT ARG_DISPLAY_XML)
  ENDIF(ARG_DISPLAY_PANEL)

  SET(${OUTIFACES} ${ARG_VIEW_TYPE})
  IF(NOT ARG_VIEW_XML_NAME)
    SET(ARG_VIEW_XML_NAME ${ARG_VIEW_TYPE})
  ENDIF(NOT ARG_VIEW_XML_NAME)
  IF(ARG_VIEW_NAME)
    SET(VIEW_TYPE_NAME ${ARG_VIEW_NAME})
  ELSE(ARG_VIEW_NAME)
    SET(VIEW_TYPE_NAME ${ARG_VIEW_TYPE})
  ENDIF(ARG_VIEW_NAME)

  IF(NOT ARG_DISPLAY_TYPE)
    SET(ARG_DISPLAY_TYPE "pqDataRepresentation")
  ENDIF(NOT ARG_DISPLAY_TYPE)

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Core/pqViewModuleImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Core/pqViewModuleImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  SET(VIEW_MOC_SRCS)
  QT4_WRAP_CPP(VIEW_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  IF(ARG_DISPLAY_PANEL)
    ADD_PARAVIEW_DISPLAY_PANEL(OUT_PANEL_IFACES PANEL_SRCS 
                               CLASS_NAME ${ARG_DISPLAY_PANEL} 
                               XML_NAME ${ARG_DISPLAY_XML})
    SET(${OUTIFACES} ${ARG_VIEW_TYPE} ${OUT_PANEL_IFACES})
  ENDIF(ARG_DISPLAY_PANEL)

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}Implementation.h
      ${VIEW_MOC_SRCS}
      ${PANEL_SRCS}
      )

ENDMACRO(ADD_PARAVIEW_VIEW_MODULE)

# create implementation for a custom view options interface
# ADD_PARAVIEW_VIEW_OPTIONS(
#    OUTIFACES
#    OUTSRCS
#    VIEW_TYPE type
#    [ACTIVE_VIEW_OPTIONS classname]
#    [GLOBAL_VIEW_OPTIONS classname]
#
#  VIEW_TYPE: the type of view the options panels are associated with
#  ACTIVE_VIEW_OPTIONS: optional name for the class that implements pqActiveViewOptions
#                       this is to add options that are specific to a view instance
#  GLOBAL_VIEW_OPTIONS: optional name for the class that implements pqOptionsContainer
#                       this is to add options that apply to all view instances
MACRO(ADD_PARAVIEW_VIEW_OPTIONS OUTIFACES OUTSRCS)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "VIEW_TYPE;ACTIVE_VIEW_OPTIONS;GLOBAL_VIEW_OPTIONS" "" ${ARGN} )
  
  IF(NOT ARG_VIEW_TYPE)
    MESSAGE(ERROR " ADD_PARAVIEW_VIEW_OPTIONS called without VIEW_TYPE")
  ENDIF(NOT ARG_VIEW_TYPE)
  
  IF(NOT ARG_ACTIVE_VIEW_OPTIONS AND NOT ARG_GLOBAL_VIEW_OPTIONS)
    MESSAGE(ERROR " ADD_PARAVIEW_VIEW_OPTIONS called without ACTIVE_VIEW_OPTIONS or GLOBAL_VIEW_OPTIONS")
  ENDIF(NOT ARG_ACTIVE_VIEW_OPTIONS AND NOT ARG_GLOBAL_VIEW_OPTIONS)
  
  SET(HAVE_ACTIVE_VIEW_OPTIONS 0)
  SET(HAVE_GLOBAL_VIEW_OPTIONS 0)
  
  IF(ARG_ACTIVE_VIEW_OPTIONS)
    SET(HAVE_ACTIVE_VIEW_OPTIONS 1)
  ENDIF(ARG_ACTIVE_VIEW_OPTIONS)
  
  IF(ARG_GLOBAL_VIEW_OPTIONS)
    SET(HAVE_GLOBAL_VIEW_OPTIONS 1)
  ENDIF(ARG_GLOBAL_VIEW_OPTIONS)

  SET(${OUTIFACES} ${ARG_VIEW_TYPE}Options)

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqViewOptionsImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}OptionsImplementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqViewOptionsImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}OptionsImplementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  SET(PANEL_MOC_SRCS)
  QT4_WRAP_CPP(PANEL_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}OptionsImplementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

 SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}OptionsImplementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_VIEW_TYPE}OptionsImplementation.h
      ${PANEL_MOC_SRCS}
      )

ENDMACRO(ADD_PARAVIEW_VIEW_OPTIONS)

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
 
  SET(${OUTIFACES} ${ARG_CLASS_NAME})

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqActionGroupImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqActionGroupImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_ACTION_GROUP)

# create implementation for a custom view frame action interface
# ADD_PARAVIEW_VIEW_FRAME_ACTION_GROUP(
#    OUTIFACES
#    OUTSRCS
#    CLASS_NAME classname
#
#    CLASS_NAME is the name of the class that implements a QActionGroup
MACRO(ADD_PARAVIEW_VIEW_FRAME_ACTION_GROUP OUTIFACES OUTSRCS)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME" "" ${ARGN} )
 
  SET(${OUTIFACES} ${ARG_CLASS_NAME})

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqViewFrameActionGroupImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqViewFrameActionGroupImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_VIEW_FRAME_ACTION_GROUP)

# create implementation for a dock window interface
# ADD_PARAVIEW_DOCK_WINDOW(
#    OUTIFACES
#    OUTSRCS
#    CLASS_NAME classname
#    [DOCK_AREA areaname]
#
#  CLASS_NAME: is the name of the class that implements a QDockWidget
#  DOCK_AREA: option to specify the dock area (Left | Right | Top | Bottom)
#             Left is the default
MACRO(ADD_PARAVIEW_DOCK_WINDOW OUTIFACES OUTSRCS)

  SET(ARG_DOCK_AREA)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;DOCK_AREA" "" ${ARGN} )

  IF(NOT ARG_DOCK_AREA)
    SET(ARG_DOCK_AREA Left)
  ENDIF(NOT ARG_DOCK_AREA)
  SET(${OUTIFACES} ${ARG_CLASS_NAME})

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqDockWindowImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqDockWindowImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_DOCK_WINDOW)


# Create implementation for an auto start interface.
# ADD_PARAVIEW_AUTO_START(
#   OUTIFACES
#   OUTSRCS
#   CLASS_NAME classname
#   [STARTUP startup callback method name]
#   [SHUTDOWN shutdown callback method name]
# )
# CLASS_NAME : is the name of the class that implements 2 methods which will be
#              called on startup and shutdown. The names of these methods can be
#              optionally specified using STARTUP and SHUTDOWN.
# STARTUP    : name of the method on class CLASS_NAME which should be called 
#              when the plugins loads. Default is startup.
# SHUTDOWN   : name pf the method on class CLASS_NAME which should be called
#              when the application shuts down. Default is shutdown.
MACRO(ADD_PARAVIEW_AUTO_START OUTIFACES OUTSRCS)
  SET(ARG_STARTUP)
  SET(ARG_SHUTDOWN)
  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;STARTUP;SHUTDOWN" "" ${ARGN})

  IF (NOT ARG_STARTUP)
    SET (ARG_STARTUP startup)
  ENDIF (NOT ARG_STARTUP)

  IF (NOT ARG_SHUTDOWN)
    SET (ARG_SHUTDOWN shutdown)
  ENDIF (NOT ARG_SHUTDOWN)
  
  SET(${OUTIFACES} ${ARG_CLASS_NAME})
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Core/pqAutoStartImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Core/pqAutoStartImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_AUTO_START)

# Create implementation for a custom display panel decorator interface.
# Decorators are used to add additional decorations to display panels.
# ADD_PARAVIEW_DISPLAY_PANEL(
#    OUTIFACES
#    OUTSRCS
#    CLASS_NAME classname
#    PANEL_TYPES type1 type2 ..)
# CLASS_NAME   : The class name for the decorator. The decorator must be a
#                QObject subclass. The display panel is passed as the parent for
#                the object.
# PANEL_TYPES  : list of classnames for the display panel which this decorator
#                can decorate.
MACRO(ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR OUTIFACES OUTSRCS)
  PV_PLUGIN_PARSE_ARGUMENTS(ARG "CLASS_NAME;PANEL_TYPES" "" ${ARGN})

  SET(${OUTIFACES} ${ARG_CLASS_NAME})
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqDisplayPanelDecoratorImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Qt/Components/pqDisplayPanelDecoratorImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  SET(ACTION_MOC_SRCS)
  QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO(ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR)


#  Macro for a GraphLayoutStrategy plugin
#  STRATEGY_TYPE = "MyStrategy"
MACRO(ADD_PARAVIEW_GRAPH_LAYOUT_STRATEGY OUTIFACES OUTSRCS)
  
  SET(ARG_STRATEGY_TYPE)
  SET(ARG_STRATEGY_LABEL)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "STRATEGY_TYPE;STRATEGY_LABEL"
                  "" ${ARGN} )

  IF(NOT ARG_STRATEGY_TYPE OR NOT ARG_STRATEGY_LABEL)
    MESSAGE(ERROR " ADD_PARAVIEW_GRAPH_LAYOUT_STRATEGY called without STRATEGY_TYPE")
  ENDIF(NOT ARG_STRATEGY_TYPE OR NOT ARG_STRATEGY_LABEL)

  SET(${OUTIFACES} ${ARG_STRATEGY_TYPE})

  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Applications/OverView/Core/pqGraphLayoutStrategyImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_SOURCE_DIR}/Applications/OverView/Core/pqGraphLayoutStrategyImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.cxx @ONLY)

  GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${QT_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS}")
  SET(LAYOUT_MOC_SRCS)
  QT4_WRAP_CPP(LAYOUT_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

  SET(${OUTSRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h
      ${LAYOUT_MOC_SRCS}
      )

ENDMACRO(ADD_PARAVIEW_GRAPH_LAYOUT_STRATEGY)

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
  SET(PLUGIN_MOC_SRCS)
  QT4_WRAP_CPP(PLUGIN_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}PluginImplementation.h)
  SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")
  
  SET(${OUTSRCS} ${PLUGIN_MOC_SRCS} 
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}PluginImplementation.cxx)

ENDMACRO(ADD_PARAVIEW_GUI_EXTENSION)

# internal macro to work around deficiency in FindQt4.cmake, will be removed in
# the future.
MACRO(PARAVIEW_QT4_ADD_RESOURCES outfiles )
  FOREACH (it ${ARGN})
    GET_FILENAME_COMPONENT(outfilename ${it} NAME_WE)
    GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
    GET_FILENAME_COMPONENT(rc_path ${infile} PATH)
    SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.cxx)
    #  parse file for dependencies 
    #  all files are absolute paths or relative to the location of the qrc file
    FILE(READ "${infile}" _RC_FILE_CONTENTS)
    STRING(REGEX MATCHALL "<file[^<]+" _RC_FILES "${_RC_FILE_CONTENTS}")
    SET(_RC_DEPENDS)
    FOREACH(_RC_FILE ${_RC_FILES})
      STRING(REGEX REPLACE "^<file[^>]*>" "" _RC_FILE "${_RC_FILE}")
      STRING(REGEX MATCH "^/|([A-Za-z]:/)" _ABS_PATH_INDICATOR "${_RC_FILE}")
      IF(NOT _ABS_PATH_INDICATOR)
        SET(_RC_FILE "${rc_path}/${_RC_FILE}")
      ENDIF(NOT _ABS_PATH_INDICATOR)
      SET(_RC_DEPENDS ${_RC_DEPENDS} "${_RC_FILE}")
    ENDFOREACH(_RC_FILE)
    ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
      COMMAND ${QT_RCC_EXECUTABLE}
      ARGS ${rcc_options} -name ${outfilename} -o ${outfile} ${infile}
      MAIN_DEPENDENCY ${infile}
      DEPENDS ${_RC_DEPENDS})
    SET(${outfiles} ${${outfiles}} ${outfile})
  ENDFOREACH (it)
ENDMACRO(PARAVIEW_QT4_ADD_RESOURCES)

# create a plugin
#  A plugin may contain only server code, only gui code, or both.
#  SERVER_MANAGER_SOURCES will be wrapped
#  SERVER_MANAGER_XML will be embedded and give to the client when loaded
#  SERVER_SOURCES is for other source files
#  PYTHON_MODULES allows you to embed python sources as modules
#  GUI_INTERFACES is to specify which GUI plugin interfaces were implemented
#  GUI_RESOURCES is to specify qrc files
#  GUI_RESOURCE_FILES is to specify xml files to create a qrc file from
#  GUI_SOURCES is to other GUI sources
#  SOURCES is deprecated, please use SERVER_SOURCES or GUI_SOURCES
# ADD_PARAVIEW_PLUGIN(Name Version
#     [SERVER_MANAGER_SOURCES source files]
#     [SERVER_MANAGER_XML XMLFile]
#     [SERVER_SOURCES source files]
#     [PYTHON_MODULES python source files]
#     [GUI_INTERFACES interface1 interface2]
#     [GUI_RESOURCES qrc1 qrc2]
#     [GUI_RESOURCE_FILES xml1 xml2]
#     [GUI_SOURCES source files]
#     [SOURCES source files]
#  )
MACRO(ADD_PARAVIEW_PLUGIN NAME VERSION)

  IF(PARAVIEW_BUILD_SHARED_LIBS)

    SET(QT_RCS)
    SET(GUI_SRCS)
    SET(SM_SRCS)
    SET(PY_SRCS)
    SET(ARG_GUI_INTERFACES)
    SET(ARG_GUI_RESOURCES)
    SET(ARG_GUI_RESOURCE_FILES)
    SET(ARG_SERVER_MANAGER_SOURCES)
    SET(ARG_SERVER_MANAGER_XML)
    SET(ARG_PYTHON_MODULES)
    SET(ARG_SOURCES)
    SET(ARG_SERVER_SOURCES)
    SET(ARG_GUI_SOURCES)
    
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

    PV_PLUGIN_PARSE_ARGUMENTS(ARG 
      "SERVER_MANAGER_SOURCES;SERVER_MANAGER_XML;SERVER_SOURCES;PYTHON_MODULES;GUI_INTERFACES;GUI_RESOURCES;GUI_RESOURCE_FILES;GUI_SOURCES;SOURCES"
      "" ${ARGN} )

    IF(ARG_SERVER_MANAGER_SOURCES OR ARG_SERVER_MANAGER_XML)
      ADD_SERVER_MANAGER_EXTENSION(SM_SRCS ${NAME} "${ARG_SERVER_MANAGER_XML}"
                                   ${ARG_SERVER_MANAGER_SOURCES})
    ENDIF(ARG_SERVER_MANAGER_SOURCES OR ARG_SERVER_MANAGER_XML)

    IF (ARG_PYTHON_MODULES)
      IF (PARAVIEW_ENABLE_PYTHON)
        ADD_PYTHON_EXTENSION(PY_SRCS ${NAME} ${ARG_PYTHON_MODULES})
      ELSE (PARAVIEW_ENABLE_PYTHON)
        MESSAGE(STATUS "Python parameters ignored for ${NAME} plugin because PARAVIEW_ENABLE_PYTHON is off.")
      ENDIF (PARAVIEW_ENABLE_PYTHON)
    ENDIF (ARG_PYTHON_MODULES)

    IF(PARAVIEW_BUILD_QT_GUI)
      IF(ARG_GUI_RESOURCE_FILES)
        SET(QT_COMPONENTS_GUI_RESOURCES_CONTENTS)
        FOREACH(RESOURCE ${ARG_GUI_RESOURCE_FILES})
          GET_FILENAME_COMPONENT(ALIAS ${RESOURCE} NAME)
          GET_FILENAME_COMPONENT(RESOURCE ${RESOURCE} ABSOLUTE)
          FILE(RELATIVE_PATH RESOURCE ${CMAKE_CURRENT_BINARY_DIR} "${RESOURCE}")
          FILE(TO_NATIVE_PATH "${RESOURCE}" RESOURCE)
          SET(QT_COMPONENTS_GUI_RESOURCES_CONTENTS
              "${QT_COMPONENTS_GUI_RESOURCES_CONTENTS} <file alias=\"${ALIAS}\">${RESOURCE}</file>\n")
        ENDFOREACH(RESOURCE)
        CONFIGURE_FILE("${ParaView_SOURCE_DIR}/Qt/Components/pqExtraResources.qrc.in"
                       "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.qrc" @ONLY)
        SET(ARG_GUI_RESOURCES ${ARG_GUI_RESOURCES}
          "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.qrc")
      ENDIF(ARG_GUI_RESOURCE_FILES)
      
      IF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES OR ARG_GUI_SOURCES)
        ADD_PARAVIEW_GUI_EXTENSION(GUI_SRCS ${NAME} INTERFACES "${ARG_GUI_INTERFACES}")
      ENDIF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES OR ARG_GUI_SOURCES)

      IF(ARG_GUI_RESOURCES)
        PARAVIEW_QT4_ADD_RESOURCES(QT_RCS ${ARG_GUI_RESOURCES})
        SET(GUI_SRCS ${GUI_SRCS} ${QT_RCS})
      ENDIF(ARG_GUI_RESOURCES)

      SET(GUI_SRCS ${GUI_SRCS} ${ARG_GUI_SOURCES})

    ELSE(PARAVIEW_BUILD_QT_GUI)

      IF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES OR ARG_GUI_RESOURCE_FILES)
        MESSAGE(STATUS "GUI parameters ignored for ${NAME} plugin because PARAVIEW_BUILD_QT_GUI is off.")
      ENDIF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES OR ARG_GUI_RESOURCE_FILES)

    ENDIF(PARAVIEW_BUILD_QT_GUI)

    SET(SM_SRCS
      ${ARG_SERVER_MANAGER_SOURCES}
      ${SM_SRCS}
      ${ARG_SERVER_SOURCES}
      ${PY_SRCS})

    IF(GUI_SRCS OR SM_SRCS OR ARG_SOURCES)
      IF(PARAVIEW_PLUGINLIST_TXT)
        FILE(APPEND ${PARAVIEW_PLUGINLIST_TXT} "${LIBRARY_OUTPUT_PATH}/lib${NAME}.dylib;")
      ENDIF(PARAVIEW_PLUGINLIST_TXT)

      ADD_LIBRARY(${NAME} SHARED ${GUI_SRCS} ${SM_SRCS} ${ARG_SOURCES})

      IF(GUI_SRCS)
        TARGET_LINK_LIBRARIES(${NAME} pqComponents)
      ENDIF(GUI_SRCS)
      IF(SM_SRCS)
        TARGET_LINK_LIBRARIES(${NAME} vtkPVServerManager)
      ENDIF(SM_SRCS)

    ENDIF(GUI_SRCS OR SM_SRCS OR ARG_SOURCES)
  
  ELSE(PARAVIEW_BUILD_SHARED_LIBS)
    MESSAGE("ParaView plugins are disabled.  Please build ParaView with shared libraries.") 
  ENDIF(PARAVIEW_BUILD_SHARED_LIBS)

ENDMACRO(ADD_PARAVIEW_PLUGIN)
