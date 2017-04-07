include(ParaViewMacros)

# Macro to install a plugin that's included in the ParaView source directory.
# This is a macro internal to ParaView and should not be directly used by
# external applications. This may change in future without notice.
MACRO(internal_paraview_install_plugin name)
  IF (PV_INSTALL_PLUGIN_DIR)
    INSTALL(TARGETS ${name}
            DESTINATION ${PV_INSTALL_PLUGIN_DIR}
            COMPONENT Runtime)
  ENDIF ()
ENDMACRO()

# helper PV_PLUGIN_LIST_CONTAINS macro
MACRO(PV_PLUGIN_LIST_CONTAINS var value)
  SET(${var})
  FOREACH (value2 ${ARGN})
    IF (${value} STREQUAL ${value2})
      SET(${var} TRUE)
    ENDIF ()
  ENDFOREACH ()
ENDMACRO()

# helper PV_PLUGIN_PARSE_ARGUMENTS macro
MACRO(PV_PLUGIN_PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})
    SET(${prefix}_${arg_name})
  ENDFOREACH()
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH()

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})
    PV_PLUGIN_LIST_CONTAINS(is_arg_name ${arg} ${arg_names})
    IF (is_arg_name)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE ()
      PV_PLUGIN_LIST_CONTAINS(is_option ${arg} ${option_names})
      IF (is_option)
        SET(${prefix}_${arg} TRUE)
      ELSE ()
        SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF ()
    ENDIF ()
  ENDFOREACH()
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO()

# Macro to encode any file(s) as a string. This creates a new cxx file with a
# declaration of a "const char*" string with the same name as the file.
# Example:
# encode_files_as_strings(cxx_files
#  vtkLightingHelper_s.glsl
#  vtkColorMaterialHelper_vs.glsl
#  )
# Will create 2 cxx files with 2 strings: const char* vtkLightingHelper_s and
# const char* vtkColorMaterialHelper_vs.
MACRO(ENCODE_FILES_AS_STRINGS OUT_SRCS)
  foreach(file ${ARGN})
    GET_FILENAME_COMPONENT(file "${file}" ABSOLUTE)
    GET_FILENAME_COMPONENT(file_name "${file}" NAME_WE)
    set(src ${file})
    set(res ${CMAKE_CURRENT_BINARY_DIR}/${file_name}.cxx)
    add_custom_command(
      OUTPUT ${res}
      DEPENDS ${src} vtkEncodeString
      COMMAND vtkEncodeString
      ARGS ${res} ${src} ${file_name}
      )
    set(${OUT_SRCS} ${${OUT_SRCS}} ${res})
  endforeach()
ENDMACRO()

# create plugin glue code for a server manager extension
# consisting of server manager XML and VTK classes
# sets OUTSRCS with the generated code
MACRO(ADD_SERVER_MANAGER_EXTENSION OUTSRCS Name Version XMLFile)
  SET (plugin_type_servermanager TRUE)
  SET (SM_PLUGIN_INCLUDES)
  SET (XML_INTERFACES_INIT)

  # if (XMLFile) doesn't work correctly in a macro. We need to
  # set a local variable.
  set (xmlfiles ${XMLFile})
  if (xmlfiles)
    # generate a header from all the xmls specified.
    set(XML_HEADER "${CMAKE_CURRENT_BINARY_DIR}/vtkSMXML_${Name}.h")

    generate_header(${XML_HEADER}
      PREFIX "${Name}"
      SUFFIX "Interfaces"
      VARIABLE function_names
      FILES ${xmlfiles})

    foreach (func_name ${function_names})
      set (XML_INTERFACES_INIT
        "${XML_INTERFACES_INIT}  PushBack(xmls, ${func_name});\n")
    endforeach()

    set (SM_PLUGIN_INCLUDES "${SM_PLUGIN_INCLUDES}#include \"${XML_HEADER}\"\n")
  endif()

  SET(HDRS)

  FOREACH(SRC ${ARGN})
    GET_FILENAME_COMPONENT(src_name "${SRC}" NAME_WE)
    GET_FILENAME_COMPONENT(src_path "${SRC}" ABSOLUTE)
    GET_FILENAME_COMPONENT(src_path "${src_path}" PATH)
    SET(HDR)
    IF(EXISTS "${src_path}/${src_name}.h")
      SET(HDR "${src_path}/${src_name}.h")
    ELSEIF(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${src_name}.h")
      SET(HDR "${CMAKE_CURRENT_BINARY_DIR}/${src_name}.h")
    ENDIF()
    LIST(APPEND HDRS ${HDR})
  ENDFOREACH()

  SET(CS_SRCS)
  IF(HDRS)
    include(vtkWrapClientServer)

    # Plugins should not use unified bindings. The problem arises because the
    # PythonD library links to the plugin itself, but the CS wrapping code
    # lives in the plugin as well. With unified bindings, the CS wrapping
    # needs to link to the PythonD library which causes a circular
    # dependency. The solution is probably to compile all bindings for a
    # plugin into a single library.
    set(NO_PYTHON_BINDINGS_AVAILABLE TRUE)
    VTK_WRAP_ClientServer(${Name} CS_SRCS "${HDRS}")
    # only generate the instantiator code for cxx classes that'll be included in
    # the plugin
    SET(INITIALIZE_WRAPPING 1)
  ELSE()
    SET(INITIALIZE_WRAPPING 0)
  ENDIF()

  SET(${OUTSRCS} ${CS_SRCS} ${XML_HEADER})

ENDMACRO()

MACRO(ADD_PYTHON_EXTENSION OUTSRCS NAME VERSION)
  SET(PYSRCFILES ${ARGN})
  SET(WRAP_PY_HEADERS)
  SET(WRAP_PYTHON_INCLUDES)
  SET(PY_MODULE_LIST)
  SET(PY_LOADER_LIST)
  SET(PY_PACKAGE_FLAGS)
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
      ELSE ()
        SET(PYFILE_MODULE "${PYFILE_PACKAGE}.${PYFILE_NAME}")
      ENDIF ()
    ELSE ()
      SET(PYFILE_MODULE "${PYFILE_NAME}")
    ENDIF ()
    STRING(REPLACE "." "_" PYFILE_MODULE_MANGLED "${PYFILE_MODULE}")
    SET(PY_HEADER "${CMAKE_CURRENT_BINARY_DIR}/WrappedPython_${NAME}_${PYFILE_MODULE_MANGLED}.h")
    ADD_CUSTOM_COMMAND(
      OUTPUT "${PY_HEADER}"
      DEPENDS "${PYFILE_ABSOLUTE}" kwProcessXML
      COMMAND kwProcessXML
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
    ELSE()
      SET(PY_MODULE_LIST "\"${PYFILE_MODULE}\"")
      SET(PY_LOADER_LIST
        "module_${PYFILE_MODULE_MANGLED}_${PYFILE_NAME}_source()")
      SET(PY_PACKAGE_FLAGS "${PACKAGE_FLAG}")
    ENDIF()
  ENDFOREACH()

  # Create source code to get Python source from the plugin.
  SET (plugin_type_python TRUE)
  SET(${OUTSRCS} "${PY_INIT_SRC}" "${WRAP_PY_HEADERS}")

ENDMACRO()


#------------------------------------------------------------------------------
# Register a custom pqPropertyWidget class for a property.
# pqPropertyWidget instances are used
# to create widgets for properties in the Properties Panel.
# Usage:
#   add_paraview_property_widget(OUTIFACES OUTSRCS
#     TYPE "<string identifier>"
#     CLASS_NAME "<classname>")
macro(add_paraview_property_widget outifaces outsrcs)
  set (pwi_widget 1)
  set (pwi_group 0)
  set (pwi_decorator 0)
  __add_paraview_property_widget(${outifaces} ${outsrcs} ${ARGN})
endmacro()

#------------------------------------------------------------------------------
# Register a custom pqPropertyWidget class for a property group.
# pqPropertyWidget instances are used to create widgets for properties in the
# Properties Panel.
# Usage:
#   add_paraview_property_group_widget(OUTIFACES OUTSRCS
#     TYPE "<string identifier>"
#     CLASS_NAME "<classname>")
macro(add_paraview_property_group_widget outifaces outsrcs)
  set (pwi_widget 0)
  set (pwi_group 1)
  set (pwi_decorator 0)
  __add_paraview_property_widget(${outifaces} ${outsrcs} ${ARGN})
endmacro()

#------------------------------------------------------------------------------
# Register a custom pqPropertyWidgetDecorator.
# pqPropertyWidgetDecorator instances are used to add custom logic to
# pqPropertyWidget.
# Usage:
#   add_paraview_property_widget_decorator(OUTIFACES OUTSRCS
#     TYPE "<string identifier>"
#     CLASS_NAME "<classname>")
macro(add_paraview_property_widget_decorator outifaces outsrcs)
  set (pwi_widget 0)
  set (pwi_group 0)
  set (pwi_decorator 1)
  __add_paraview_property_widget(${outifaces} ${outsrcs} ${ARGN})
endmacro()

#------------------------------------------------------------------------------
# Internal function used by add_paraview_property_widget,
# add_paraview_property_group_widget and add_paraview_property_widget_decorator
function(__add_paraview_property_widget outifaces outsrcs)
  set (_type)
  set (_classname)

  set (_doing)
  foreach (arg ${ARGN})
    if (NOT _doing AND (arg MATCHES "^(TYPE|CLASS_NAME)$"))
      set (_doing "${arg}")
    elseif ("${_doing}" STREQUAL "TYPE")
      set (_type "${arg}")
      set (_doing)
    elseif ("${_doing}" STREQUAL "CLASS_NAME")
      set (_classname "${arg}")
      set (_doing)
    else()
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    endif()
  endforeach()

  if (_type AND _classname)
    set (name ${_classname}PWI)
    set (type ${_type})
    set (classname ${_classname})
    configure_file(${ParaView_CMAKE_DIR}/pqPropertyWidgetInterface.h.in
                   ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.h
                   @ONLY)
    configure_file(${ParaView_CMAKE_DIR}/pqPropertyWidgetInterface.cxx.in
                   ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.cxx
                   @ONLY)

    set (_moc_srcs)
    if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
      qt5_wrap_cpp(_moc_srcs ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.h)
    else ()
      qt4_wrap_cpp(_moc_srcs ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.h)
    endif ()
    set (${outifaces} ${name} PARENT_SCOPE)
    set (${outsrcs}
         ${_moc_srcs}
         ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.cxx
         ${CMAKE_CURRENT_BINARY_DIR}/${name}Implementation.h
         PARENT_SCOPE)
  else()
    message(AUTHOR_WARNING "Missing required arguments.")
  endif()
endfunction()

# OBSOLETE: legacy object panels
MACRO(ADD_PARAVIEW_OBJECT_PANEL OUTIFACES OUTSRCS)
  message(FATAL_ERROR
"ADD_PARAVIEW_OBJECT_PANEL is no longer supported.
ParaView's Properties panel has been refactored in 3.98. Legacy object panel support
was dropped in 5.2. Please refer to 'Major API Changes' in ParaView developer
documentation for details.")
ENDMACRO()

# OBSOLETE: legacy display panels.
MACRO(ADD_PARAVIEW_DISPLAY_PANEL OUTIFACES OUTSRCS)
  message(FATAL_ERROR
"ADD_PARAVIEW_DISPLAY_PANEL is no longer supported.
ParaView's Properties panel has been refactored in 3.98. Legacy display panel support
was dropped in 5.2. Please refer to 'Major API Changes' in ParaView developer
documentation for details.")
ENDMACRO()

#------------------------------------------------------------------------------
# Register a pqProxy subclass with ParaView. This macro is used to register
# pqProxy subclasses, including pqView subclasses, pqDataRepresentation
# subclasses, etc. to create when a particular type of proxy is registered with
# the application.
# Usage:
#   add_pqproxy(OUTIFACES OUTSRCS
#     TYPE <pqProxy subclass name>
#     XML_GROUP <xml group used to identify the vtkSMProxy>
#     XML_NAME <xml name used to indentify the vtkSMProxy>
#     ...)
# The TYPE, XML_GROUP, and XML_NAME can be repeated to register multiple types
# of pqProxy subclasses or reuse the same pqProxy for multiple proxy types.
macro(add_pqproxy OUTIFACES OUTSRCS)
  set (arg_types)
  set (_doing)
  set (_active_index)
  foreach (arg ${ARGN})
    if ((NOT _doing) AND ("${arg}" MATCHES "^(TYPE|XML_GROUP|XML_NAME)$"))
      set (_doing "${arg}")
    elseif (_doing STREQUAL "TYPE")
      list(APPEND arg_types "${arg}")
      list(LENGTH arg_types _active_index)
      math(EXPR _active_index "${_active_index}-1")
      set (_type_${_active_index}_xmlgroup)
      set (_type_${_active_index}_xmlname)
      set (_doing)
    elseif (_doing STREQUAL "XML_GROUP")
      set (_type_${_active_index}_xmlgroup "${arg}")
      set (_doing)
    elseif (_doing STREQUAL "XML_NAME")
      set (_type_${_active_index}_xmlname "${arg}")
      set (_doing)
    else()
      set (_doing)
    endif()
  endforeach()

  list(LENGTH arg_types num_items)
  math(EXPR max_index "${num_items}-1")
  set (ARG_INCLUDES)
  set (ARG_BODY)
  foreach (index RANGE ${max_index})
    list(GET arg_types ${index} arg_type)
    set (arg_xml_group "${_type_${index}_xmlgroup}")
    set (arg_xml_name "${_type_${index}_xmlname}")
    set (ARG_INCLUDES "${ARG_INCLUDES}#include\"${arg_type}.h\"\n")
    set (ARG_BODY "${ARG_BODY}
    if (QString(\"${arg_xml_group}\") == proxy->GetXMLGroup() &&
        QString(\"${arg_xml_name}\") == proxy->GetXMLName())
        {
        return new ${arg_type}(regGroup, regName, proxy, server, NULL);
        }")
  endforeach()

  if (ARG_INCLUDES AND ARG_BODY)
    list(GET arg_types 0 ARG_TYPE)
    set (IMP_CLASS "${ARG_TYPE}ServerManagerModelImplementation")
    configure_file(${ParaView_CMAKE_DIR}/pqServerManagerModelImplementation.h.in
      ${CMAKE_CURRENT_BINARY_DIR}/${IMP_CLASS}.h @ONLY)
    configure_file(${ParaView_CMAKE_DIR}/pqServerManagerModelImplementation.cxx.in
      ${CMAKE_CURRENT_BINARY_DIR}/${IMP_CLASS}.cxx @ONLY)

    set (_moc_srcs)
    if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
      QT5_WRAP_CPP(_moc_srcs ${CMAKE_CURRENT_BINARY_DIR}/${IMP_CLASS}.h)
    else()
      QT4_WRAP_CPP(_moc_srcs ${CMAKE_CURRENT_BINARY_DIR}/${IMP_CLASS}.h)
    endif()

    set(${OUTIFACES} ${${OUTIFACES}} ${ARG_TYPE}ServerManagerModel) # don't add
                                        # the extra "Implementation" here.
    set(${OUTSRCS}
      ${${OUTSRCS}}
      ${_moc_srcs}
      ${CMAKE_CURRENT_BINARY_DIR}/${IMP_CLASS}.h
      ${CMAKE_CURRENT_BINARY_DIR}/${IMP_CLASS}.cxx
      )
  endif()

  unset (ARG_TYPE)
  unset (ARG_INCLUDES)
  unset (ARG_BODY)
endmacro()

#------------------------------------------------------------------------------
# *** OBSOLETE *** : No longer supported.
# To add new view proxies (or representation proxies) simply add new proxies to
# "views" or "representations" groups. To add new pqView or pqDataRepresentation
# subclasses, use ADD_PQPROXY().
# create implementation for a custom view
# Obsolete Usage:
# ADD_PARAVIEW_VIEW_MODULE( OUTIFACES OUTSRCS
#     VIEW_TYPE Type
#     VIEW_XML_GROUP Group
#     [VIEW_XML_NAME Name]
#     [VIEW_NAME Name]
#     [DISPLAY_PANEL Display]
#     [DISPLAY_TYPE Display]
MACRO(ADD_PARAVIEW_VIEW_MODULE OUTIFACES OUTSRCS)
  message(FATAL_ERROR
"'ADD_PARAVIEW_VIEW_MODULE' macro is no longer supported.  To add new view proxies, or representation proxies, simply add new proxies to 'views' or 'representations' groups. To add new pqView or pqDataRepresentation subclasses, use 'ADD_PQPROXY' macro.")
ENDMACRO()

#------------------------------------------------------------------------
# OBSOLETE: create implementation for a custom view options interface
MACRO(ADD_PARAVIEW_VIEW_OPTIONS)
  message(FATAL_ERROR
"'ADD_PARAVIEW_VIEW_OPTIONS' macro is no longer supported.
ParaView's settings/view settings infrastructure has been refactored.
These old options panel no longer make sense and hence cannot be supported
anymore.")
ENDMACRO()
#------------------------------------------------------------------------

# create implementation for a custom menu or toolbar
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

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqActionGroupImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqActionGroupImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  SET(ACTION_MOC_SRCS)
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  ELSE ()
    QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  ENDIF ()

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO()

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

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqViewFrameActionGroupImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqViewFrameActionGroupImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  SET(ACTION_MOC_SRCS)
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  ELSE ()
    QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  ENDIF ()

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO()

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
  ENDIF()
  SET(${OUTIFACES} ${ARG_CLASS_NAME})

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqDockWindowImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqDockWindowImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  SET(ACTION_MOC_SRCS)
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  ELSE ()
    QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  ENDIF ()

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO()


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
  ENDIF ()

  IF (NOT ARG_SHUTDOWN)
    SET (ARG_SHUTDOWN shutdown)
  ENDIF ()

  SET(${OUTIFACES} ${ARG_CLASS_NAME})
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqAutoStartImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqAutoStartImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx @ONLY)

  SET(ACTION_MOC_SRCS)
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  ELSE ()
    QT4_WRAP_CPP(ACTION_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h)
  ENDIF ()

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_CLASS_NAME}Implementation.h
      ${ACTION_MOC_SRCS}
      )
ENDMACRO()

#--------------------------------------------------------------------------------------
# OBSOLETE: Create implementation for a custom display panel decorator interface.
# Decorators are used to add additional decorations to display panels.
MACRO(ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR)
  message(FATAL_ERROR
"'ADD_PARAVIEW_DISPLAY_PANEL_DECORATOR' macro is no longer supported.
ParaView's Properties panel has been refactored in 3.98.
Display Panel Decorators are no longer applicaple.")
ENDMACRO()


#--------------------------------------------------------------------------------------
# OBSOLETE: 3DWidgets are simply custom property panels (pqPropertyWidget
# subclasses). Thus, use add_paraview_property_group_widget() to resgiter a new
# 3D widget panel after having updated the code accordingly.
# Creates implementation for a pq3DWidgetInterface to add new 3D widgets to ParaView.
MACRO(ADD_3DWIDGET OUTIFACES OUTSRCS)
  message(FATAL_ERROR
"'ADD_3DWIDGET' macro is no longer supported.
ParaView's Properties panel has been refactored in 3.98. Legacy 3DWidget support
was dropped in 5.1. Please refer to 'Major API Changes' in ParaView developer
documentation for details.")
ENDMACRO()


#  Macro for a GraphLayoutStrategy plugin
#  STRATEGY_TYPE = "MyStrategy"
MACRO(ADD_PARAVIEW_GRAPH_LAYOUT_STRATEGY OUTIFACES OUTSRCS)

  SET(ARG_STRATEGY_TYPE)
  SET(ARG_STRATEGY_LABEL)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "STRATEGY_TYPE;STRATEGY_LABEL"
                  "" ${ARGN} )

  IF(NOT ARG_STRATEGY_TYPE OR NOT ARG_STRATEGY_LABEL)
    MESSAGE(ERROR " ADD_PARAVIEW_GRAPH_LAYOUT_STRATEGY called without STRATEGY_TYPE")
  ENDIF()

  SET(${OUTIFACES} ${ARG_STRATEGY_TYPE})

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqGraphLayoutStrategyImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqGraphLayoutStrategyImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.cxx @ONLY)

  SET(LAYOUT_MOC_SRCS)
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(LAYOUT_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h)
  ELSE ()
    QT4_WRAP_CPP(LAYOUT_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h)
  ENDIF ()

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h
      ${LAYOUT_MOC_SRCS}
      )

ENDMACRO()

#  Macro for a AreaLayoutStrategy plugin
#  STRATEGY_TYPE = "MyStrategy"
MACRO(ADD_PARAVIEW_TREE_LAYOUT_STRATEGY OUTIFACES OUTSRCS)

  SET(ARG_STRATEGY_TYPE)
  SET(ARG_STRATEGY_LABEL)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "STRATEGY_TYPE;STRATEGY_LABEL"
                  "" ${ARGN} )

  IF(NOT ARG_STRATEGY_TYPE OR NOT ARG_STRATEGY_LABEL)
    MESSAGE(ERROR " ADD_PARAVIEW_TREE_LAYOUT_STRATEGY called without STRATEGY_TYPE")
  ENDIF()

  SET(${OUTIFACES} ${ARG_STRATEGY_TYPE})

  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqTreeLayoutStrategyImplementation.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h @ONLY)
  CONFIGURE_FILE(${ParaView_CMAKE_DIR}/pqTreeLayoutStrategyImplementation.cxx.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.cxx @ONLY)

  SET(LAYOUT_MOC_SRCS)
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(LAYOUT_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h)
  ELSE ()
    QT4_WRAP_CPP(LAYOUT_MOC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h)
  ENDIF ()

  SET(${OUTSRCS}
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${ARG_STRATEGY_TYPE}Implementation.h
      ${LAYOUT_MOC_SRCS}
      )

ENDMACRO()

# create implementation for a Qt/ParaView plugin given a
# module name and a list of interfaces
# ADD_PARAVIEW_GUI_EXTENSION(OUTSRCS NAME VERSION INTERFACES iface1;iface2;iface3)
MACRO(ADD_PARAVIEW_GUI_EXTENSION OUTSRCS NAME VERSION)
  SET (plugin_type_gui TRUE)
  SET(INTERFACE_INCLUDES)
  SET(PUSH_BACK_PV_INTERFACES "#define PUSH_BACK_PV_INTERFACES(arg)\\\n")
  SET(ARG_INTERFACES)

  PV_PLUGIN_PARSE_ARGUMENTS(ARG "INTERFACES" "" ${ARGN} )

  IF(ARG_INTERFACES)
    FOREACH(IFACE ${ARG_INTERFACES})
      SET(TMP "#include \"${IFACE}Implementation.h\"")
      SET(INTERFACE_INCLUDES "${INTERFACE_INCLUDES}\n${TMP}")
      SET(TMP "  arg.push_back(new ${IFACE}Implementation(this));\\\n")
      SET(PUSH_BACK_PV_INTERFACES "${PUSH_BACK_PV_INTERFACES}${TMP}")
    ENDFOREACH()
  ENDIF()
  SET (PUSH_BACK_PV_INTERFACES "${PUSH_BACK_PV_INTERFACES}\n")

  SET(${OUTSRCS} ${PLUGIN_MOC_SRCS})

ENDMACRO()

# create a plugin
#  A plugin may contain only server code, only gui code, or both.
#  SERVER_MANAGER_SOURCES will be wrapped
#  SERVER_MANAGER_XML will be embedded and give to the client when loaded
#  SERVER_SOURCES is for other source files
#  PYTHON_MODULES allows you to embed python sources as modules
#  GUI_INTERFACES is to specify which GUI plugin interfaces were implemented
#  GUI_RESOURCES is to specify qrc files
#  GUI_RESOURCE_FILES warns about removed behavoir
#  GUI_SOURCES is to other GUI sources
#  SOURCES is deprecated, please use SERVER_SOURCES or GUI_SOURCES
#  REQUIRED_ON_SERVER is to specify whether this plugin should be loaded on server
#  REQUIRED_ON_CLIENT is to specify whether this plugin should be loaded on client
#  REQUIRED_PLUGINS is to specify the plugin names that this plugin depends on
#  CS_KITS is experimental option to add wrapped kits. This may change in
#  future.
#  DOCUMENTATION_DIR (optional) :- used to specify a directory containing
#  html/css/png/jpg files that comprise of the documentation for the plugin. In
#  addition, CMake will automatically generate documentation for any proxies
#  defined in XMLs for this plugin.
# ADD_PARAVIEW_PLUGIN(Name Version
#     [DOCUMENTATION_DIR dir]
#     [SERVER_MANAGER_SOURCES source files]
#     [SERVER_MANAGER_XML XMLFile]
#     [SERVER_SOURCES source files]
#     [PYTHON_MODULES python source files]
#     [GUI_INTERFACES interface1 interface2]
#     [GUI_RESOURCES qrc1 qrc2]
#     [GUI_RESOURCE_FILES xml1 xml2]
#     [GUI_SOURCES source files]
#     [SOURCES source files]
#     [REQUIRED_ON_SERVER]
#     [REQUIRED_ON_CLIENT]
#     [REQUIRED_PLUGINS pluginname1 pluginname2]
#     [CS_KITS kit1 kit2...]
#     [EXCLUDE_FROM_DEFAULT_TARGET]
#  )
FUNCTION(ADD_PARAVIEW_PLUGIN NAME VERSION)
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
  SET(ARG_REQUIRED_PLUGINS)
  SET(ARG_AUTOLOAD)
  SET(ARG_CS_KITS)
  SET(ARG_DOCUMENTATION_DIR)

  SET(PLUGIN_NAME "${NAME}")
  SET(PLUGIN_VERSION "${VERSION}")
  SET(PLUGIN_REQUIRED_ON_SERVER 1)
  SET(PLUGIN_REQUIRED_ON_CLIENT 1)
  SET(PLUGIN_REQUIRED_PLUGINS)
  SET(HAVE_REQUIRED_PLUGINS 0)
  SET(BINARY_RESOURCES_INIT)
  SET(QRC_RESOURCES_INIT)
  SET(EXTRA_INCLUDES)

  # binary_resources are used to compile in icons and documentation for the
  SET(PLUGIN_EXCLUDE_FROM_DEFAULT_TARGET 0)
  # plugin. Note that this is not used to compile Qt resources, these are
  # directly compiled into the Qt plugin.
  # (since we don't support icons right now, this is used only for
  # documentation.
  set (binary_resources)


  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

  PV_PLUGIN_PARSE_ARGUMENTS(ARG
    "DOCUMENTATION_DIR;SERVER_MANAGER_SOURCES;SERVER_MANAGER_XML;SERVER_SOURCES;PYTHON_MODULES;GUI_INTERFACES;GUI_RESOURCES;GUI_RESOURCE_FILES;GUI_SOURCES;SOURCES;REQUIRED_PLUGINS;REQUIRED_ON_SERVER;REQUIRED_ON_CLIENT;EXCLUDE_FROM_DEFAULT_TARGET;AUTOLOAD;CS_KITS"
    "" ${ARGN} )

  PV_PLUGIN_LIST_CONTAINS(reqired_server_arg "REQUIRED_ON_SERVER" ${ARGN})
  PV_PLUGIN_LIST_CONTAINS(reqired_client_arg "REQUIRED_ON_CLIENT" ${ARGN})
  IF (reqired_server_arg)
    IF (NOT reqired_client_arg)
      SET(PLUGIN_REQUIRED_ON_CLIENT 0)
    ENDIF ()
  ELSE ()
    IF (reqired_client_arg)
      SET(PLUGIN_REQUIRED_ON_SERVER 0)
    ENDIF ()
  ENDIF ()

  PV_PLUGIN_LIST_CONTAINS(exclude_from_default_target_arg "EXCLUDE_FROM_DEFAULT_TARGET" ${ARGN})
  IF (exclude_from_default_target_arg)
    SET(PLUGIN_EXCLUDE_FROM_DEFAULT_TARGET 1)
  ENDIF ()

  IF(ARG_REQUIRED_PLUGINS)
    SET(PLUGIN_REQUIRED_PLUGINS "${ARG_REQUIRED_PLUGINS}")
    SET(HAVE_REQUIRED_PLUGINS 1)
  ENDIF()

  IF(ARG_SERVER_MANAGER_SOURCES OR ARG_SERVER_MANAGER_XML)
    ADD_SERVER_MANAGER_EXTENSION(SM_SRCS ${NAME} ${VERSION} "${ARG_SERVER_MANAGER_XML}"
                                 ${ARG_SERVER_MANAGER_SOURCES})
    set (EXTRA_INCLUDES "${EXTRA_INCLUDES}${SM_PLUGIN_INCLUDES}\n")
  ENDIF()

  IF (ARG_PYTHON_MODULES)
    IF (PARAVIEW_ENABLE_PYTHON)
      ADD_PYTHON_EXTENSION(PY_SRCS ${NAME} ${VERSION} ${ARG_PYTHON_MODULES})
    ELSE ()
      MESSAGE(STATUS "Python parameters ignored for ${NAME} plugin because PARAVIEW_ENABLE_PYTHON is off.")
    ENDIF ()
  ENDIF ()

  IF(PARAVIEW_BUILD_QT_GUI)
    # if server-manager xmls are specified, we can generate documentation from
    # them, if Qt is enabled.
    if (ARG_SERVER_MANAGER_XML)
      generate_htmls_from_xmls(proxy_documentation_files
        "${ARG_SERVER_MANAGER_XML}"
        "" # FIXME: not sure here. How to deal with this for plugins?
        "${CMAKE_CURRENT_BINARY_DIR}/doc")
    endif()

    # generate the qch file for the plugin if any documentation is provided.
    if (proxy_documentation_files
        OR (ARG_DOCUMENTATION_DIR AND IS_DIRECTORY "${ARG_DOCUMENTATION_DIR}"))
      build_help_project(${NAME}
        DESTINATION_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc"
        DOCUMENTATION_SOURCE_DIR "${ARG_DOCUMENTATION_DIR}"
        FILEPATTERNS "*.html;*.css;*.png;*.jpg"
        DEPENDS "${proxy_documentation_files}" )

      if (PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)
        # we don't compile the help project as a Qt resource. Instead it's
        # packaged as a SM resource. This makes it possible for
        # server-only plugins to provide documentation to the client without
        generate_header("${CMAKE_CURRENT_BINARY_DIR}/${NAME}_doc.h"
          SUFFIX "_doc"
          VARIABLE function_names
          BINARY
          FILES "${CMAKE_CURRENT_BINARY_DIR}/doc/${NAME}.qch")
        list(APPEND binary_resources ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_doc.h)
        set (EXTRA_INCLUDES "${EXTRA_INCLUDES}#include \"${CMAKE_CURRENT_BINARY_DIR}/${NAME}_doc.h\"")
      endif()
      foreach (func_name ${function_names})
        set (BINARY_RESOURCES_INIT
          "${BINARY_RESOURCES_INIT}  PushBack(resources, ${func_name});\n")
      endforeach()
    endif()

    IF(ARG_GUI_RESOURCE_FILES)
        message(WARNING "GUI resource files in plugins are no longer supported. The same"
                " functionality can be obtained using Hints in the Server Manager xml files."
                "  See the Major API Changes document for details.")
    ENDIF()

    IF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES OR ARG_GUI_SOURCES)
      ADD_PARAVIEW_GUI_EXTENSION(GUI_SRCS ${NAME} ${VERSION} INTERFACES "${ARG_GUI_INTERFACES}")
    ENDIF()

    IF(ARG_GUI_RESOURCES)
      # When building statically, we need to add stub to initialize the Qt
      # resources otherwise icons, GUI configuration xmls, etc. don't get
      # loaded/initialized when the plugin is statically imported.
      if (NOT PARAVIEW_BUILD_SHARED_LIBS)
        foreach (qrc_file IN LISTS ARG_GUI_RESOURCES)
          get_filename_component(rc_name "${qrc_file}" NAME_WE)
          set (QRC_RESOURCES_INIT
            "${QRC_RESOURCES_INIT}Q_INIT_RESOURCE(${rc_name});\n")
        endforeach()
      endif()
      pv_qt_add_resources(QT_RCS ${ARG_GUI_RESOURCES})
      SET(GUI_SRCS ${GUI_SRCS} ${QT_RCS})
    ENDIF()

    SET(GUI_SRCS ${GUI_SRCS} ${ARG_GUI_SOURCES})

  ELSE()

    IF(ARG_GUI_INTERFACES OR ARG_GUI_RESOURCES OR ARG_GUI_RESOURCE_FILES)
      MESSAGE(STATUS "GUI parameters ignored for ${NAME} plugin because PARAVIEW_BUILD_QT_GUI is off.")
    ENDIF()

  ENDIF()

  SET(SM_SRCS
    ${binary_resources}
    ${ARG_SERVER_MANAGER_SOURCES}
    ${SM_SRCS}
    ${ARG_SERVER_SOURCES}
    ${PY_SRCS})

  set (extradependencies)

  SET (PLUGIN_EXTRA_CS_INITS)
  SET (PLUGIN_EXTRA_CS_INITS_EXTERNS)
  SET (INITIALIZE_EXTRA_CS_MODULES)
  IF (ARG_CS_KITS)
    FOREACH(kit ${ARG_CS_KITS})
      SET (PLUGIN_EXTRA_CS_INITS
        "${kit}CS_Initialize(interp);\n${PLUGIN_EXTRA_CS_INITS}")
      SET (PLUGIN_EXTRA_CS_INITS_EXTERNS
        "extern \"C\" void ${kit}CS_Initialize(vtkClientServerInterpreter*);\n${PLUGIN_EXTRA_CS_INITS_EXTERNS}")
    ENDFOREACH()

    SET (INITIALIZE_EXTRA_CS_MODULES TRUE)
  ENDIF ()

  # If this plugin is being built as a part of an environment that provdes other
  # modules, we handle those.
  if (pv-plugin AND ${pv-plugin}_CS_MODULES)
    foreach(module ${${pv-plugin}_CS_MODULES})
      set (PLUGIN_EXTRA_CS_INITS
           "${module}CS_Initialize(interp);\n${PLUGIN_EXTRA_CS_INITS}")
      set (PLUGIN_EXTRA_CS_INITS_EXTERNS
           "extern \"C\" void ${module}CS_Initialize(vtkClientServerInterpreter*);\n${PLUGIN_EXTRA_CS_INITS_EXTERNS}")
      list(APPEND extradependencies ${module} ${module}CS)
    endforeach()
    set(INITIALIZE_EXTRA_CS_MODULES TRUE)
  endif()

  IF(GUI_SRCS OR SM_SRCS OR ARG_SOURCES OR ARG_PYTHON_MODULES)
    if(PARAVIEW_QT_VERSION VERSION_GREATER "4")
    else()
      set(plugin_type_gui_qt4 TRUE)
    endif()
    CONFIGURE_FILE(
      ${ParaView_CMAKE_DIR}/pqParaViewPlugin.h.in
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.h @ONLY)
    CONFIGURE_FILE(
      ${ParaView_CMAKE_DIR}/pqParaViewPlugin.cxx.in
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.cxx @ONLY)
    unset(plugin_type_gui_qt4)

    SET (plugin_sources
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.cxx
      ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.h
    )
    IF (plugin_type_gui)
      set (__plugin_sources_tmp)
      IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
        QT5_WRAP_CPP(__plugin_sources_tmp ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.h)
      ELSE ()
        QT4_WRAP_CPP(__plugin_sources_tmp ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_NAME}_Plugin.h)
      ENDIF ()
      SET (plugin_sources ${plugin_sources} ${__plugin_sources_tmp})
    ENDIF ()

   if (MSVC)
      # Do not generate manifests for the plugins - caused issues loading plugins
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
    endif()

    IF (PARAVIEW_BUILD_SHARED_LIBS)
      IF (PLUGIN_EXCLUDE_FROM_DEFAULT_TARGET)
        ADD_LIBRARY(${NAME} SHARED EXCLUDE_FROM_ALL ${GUI_SRCS} ${SM_SRCS} ${ARG_SOURCES} ${plugin_sources})
      ELSE ()
        ADD_LIBRARY(${NAME} SHARED ${GUI_SRCS} ${SM_SRCS} ${ARG_SOURCES} ${plugin_sources})
      ENDIF()
    ELSE ()
      IF (PLUGIN_EXCLUDE_FROM_DEFAULT_TARGET)
        ADD_LIBRARY(${NAME} EXCLUDE_FROM_ALL ${GUI_SRCS} ${SM_SRCS} ${ARG_SOURCES} ${plugin_sources})
      ELSE()
        ADD_LIBRARY(${NAME} ${GUI_SRCS} ${SM_SRCS} ${ARG_SOURCES} ${plugin_sources})
      ENDIF()
      # When building plugins for static builds, Qt requires this flag to be
      # defined. If not defined, when we link the executable against all the
      # plugins, we get redefinied symbols from the plugins.
      set_target_properties(${NAME} PROPERTIES
                                    COMPILE_DEFINITIONS QT_STATICPLUGIN)
    ENDIF ()

    IF(MSVC)
      # Do not generate manifests for the plugins - caused issues loading plugins
      set_target_properties(${NAME} PROPERTIES LINK_FLAGS "/MANIFEST:NO")
    ENDIF()

    IF(plugin_type_gui OR GUI_SRCS)
      target_link_libraries(${NAME}
        LINK_PUBLIC pqComponents)
    ENDIF()
    IF(SM_SRCS)
      target_link_libraries(${NAME} LINK_PUBLIC vtkPVServerManagerApplication
        vtkPVAnimation
        vtkPVServerManagerDefault
        vtkPVServerManagerApplicationCS)
    ENDIF()

    if (extradependencies)
      target_link_libraries(${NAME} LINK_PUBLIC ${extradependencies})
    endif()

    # Add install rules for the plugin. Currently only the plugins in ParaView
    # source are installed.
    internal_paraview_install_plugin(${NAME})

    IF(ARG_AUTOLOAD)
      message(WARNING "AUTOLOAD option is obsolete. Plugins built within"
        " ParaView source should use pv_plugin(..) macro with AUTOLOAD argument.")
    ENDIF()
  ENDIF()

ENDFUNCTION()

# wrap a Plugin into Python so that it can be called from pvclient and pvbatch
#it will produce lib${NAME}Python.so, which you can then
#import in your python script before calling servermanager.LoadPlugin to get
#python access to the classes from the plugin
MACRO(WRAP_PLUGIN_FOR_PYTHON NAME WRAP_LIST WRAP_EXCLUDE_LIST)
  #this was taken from Servers/ServerManager/CMakeLists.txt.
  #I did the same setup and then just inlined the call to
  #VTK/Common/KitCommonPythonWrapBlock so that plugin's name
  #does not to start with "vtk".

  SET_SOURCE_FILES_PROPERTIES(
    ${WRAP_EXCLUDE_LIST}
    WRAP_EXCLUDE)

  SET(Kit_PYTHON_EXTRA_SRCS)

  SET(KIT_PYTHON_LIBS
    vtkPVServerManagerCorePythonD
    ${NAME})

  # Tell vtkWrapPython.cmake to set VTK_PYTHON_LIBRARIES for us.
  SET(VTK_WRAP_PYTHON_FIND_LIBS 1)
  INCLUDE("${VTK_CMAKE_DIR}/vtkWrapPython.cmake")
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_DIRS})
  SET(KIT_PYTHON_DEPS)
  SET(VTK_INSTALL_NO_LIBRARIES 1)
  IF(VTKPythonWrapping_INSTALL_BIN_DIR)
    SET(VTK_INSTALL_NO_LIBRARIES)
  ENDIF()

  SET(VTK_INSTALL_LIB_DIR_CM24 "${VTKPythonWrapping_INSTALL_LIB_DIR}")
  SET(VTK_INSTALL_BIN_DIR_CM24 "${VTKPythonWrapping_INSTALL_BIN_DIR}")

  #INCLUDE(KitCommonPythonWrapBlock) takes over here
  # Create custom commands to generate the python wrappers for this kit.
  VTK_WRAP_PYTHON3(${NAME}Python KitPython_SRCS "${WRAP_LIST}")

  # Create a shared library containing the python wrappers.  Executables
  # can link to this but it is not directly loaded dynamically as a
  # module.
  ADD_LIBRARY(${NAME}PythonD ${KitPython_SRCS} ${Kit_PYTHON_EXTRA_SRCS})
  TARGET_LINK_LIBRARIES(${NAME}PythonD ${NAME} ${KIT_PYTHON_LIBS})
  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL(TARGETS ${NAME}PythonD
      RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ENDIF()
  SET(KIT_LIBRARY_TARGETS ${KIT_LIBRARY_TARGETS} ${NAME}PythonD)

  # On some UNIX platforms the python library is static and therefore
  # should not be linked into the shared library.  Instead the symbols
  # are exported from the python executable so that they can be used by
  # shared libraries that are linked or loaded.  On Windows and OSX we
  # want to link to the python libray to resolve its symbols
  # immediately.
  IF(WIN32 OR APPLE)
    TARGET_LINK_LIBRARIES (${NAME}PythonD ${VTK_PYTHON_LIBRARIES})
  ENDIF()

  # Add dependencies that may have been generated by VTK_WRAP_PYTHON3 to
  # the python wrapper library.  This is needed for the
  # pre-custom-command hack in Visual Studio 6.
  IF(KIT_PYTHON_DEPS)
    ADD_DEPENDENCIES(${NAME}PythonD ${KIT_PYTHON_DEPS})
  ENDIF()

  # Create a python module that can be loaded dynamically.  It links to
  # the shared library containing the wrappers for this kit.
  PYTHON_ADD_MODULE(${NAME}Python ${NAME}PythonInit.cxx)
  IF(PYTHON_ENABLE_MODULE_${NAME}Python)
    TARGET_LINK_LIBRARIES(${NAME}Python ${NAME}PythonD)

    # Python extension modules on Windows must have the extension ".pyd"
    # instead of ".dll" as of Python 2.5.  Older python versions do support
    # this suffix.
    IF(WIN32 AND NOT CYGWIN)
      SET_TARGET_PROPERTIES(${NAME}Python PROPERTIES SUFFIX ".pyd")
    ENDIF()

    # The python modules are installed by a setup.py script which does
    # not know how to adjust the RPATH field of the binary.  Therefore
    # we must simply build the modules with no RPATH at all.  The
    # vtkpython executable in the build tree should have the needed
    # RPATH anyway.
    SET_TARGET_PROPERTIES(${NAME}Python PROPERTIES SKIP_BUILD_RPATH 1)

    IF(WIN32 OR APPLE)
      TARGET_LINK_LIBRARIES (${NAME}Python ${VTK_PYTHON_LIBRARIES})
    ENDIF()

    # Install the extension module at the same location as other libraries.
    IF (NOT VTK_INSTALL_NO_LIBRARIES)
      INSTALL(TARGETS ${NAME}Python
        RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
        LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
        ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
    ENDIF ()
  ENDIF()

ENDMACRO()

#------------------------------------------------------------------------------
# locates module.cmake files under the current source directory and registers
# them as modules. All identified modules are treated as enabled and are built.
macro(pv_process_modules)
  if (VTK_WRAP_PYTHON)
    # this is needed to ensure that the PYTHON_INCLUDE_DIRS variable is set when
    # we process the plugins.
    find_package(PythonLibs)
  endif()

  unset (VTK_MODULES_ALL)
  file(GLOB_RECURSE files RELATIVE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/module.cmake")
  foreach (module_cmake IN LISTS files)
    get_filename_component(base "${module_cmake}" PATH)
    vtk_add_module(
      "${CMAKE_CURRENT_SOURCE_DIR}/${base}"
      module.cmake
      "${CMAKE_CURRENT_BINARY_DIR}/${base}"
      ${_test_languages})
  endforeach()

  set (current_module_set ${VTK_MODULES_ALL})
  list(APPEND VTK_MODULES_ENABLED ${VTK_MODULES_ALL})

  # sort the modules based on depedencies. This will endup bringing in
  # VTK-modules too. We raise errors if required VTK modules are not already
  # enabled.
  include(TopologicalSort)
  topological_sort(VTK_MODULES_ALL "" _DEPENDS)

  set (current_module_set_sorted)
  foreach(module IN LISTS VTK_MODULES_ALL)
    list(FIND current_module_set ${module} _found)
    if (_found EQUAL -1)
      # this is a VTK module and must have already been enabled. Otherwise raise
      # error.
      list(FIND VTK_MODULES_ENABLED ${module} _found)
      if (_found EQUAL -1)
        message(FATAL_ERROR
          "Requested modules not available: ${module}")
      endif()
    else ()
      list(APPEND current_module_set_sorted ${module})
    endif ()
  endforeach()

  set (plugin_cs_modules)
  foreach(_module IN LISTS current_module_set_sorted)
    if (NOT ${_module}_IS_TEST)
      set(vtk-module ${_module})
    else()
      set(vtk-module ${${_module}_TESTS_FOR})
    endif()
    add_subdirectory("${${_module}_SOURCE_DIR}" "${${_module}_BINARY_DIR}")
    if (NOT ${_module}_EXCLUDE_FROM_WRAPPING AND
        NOT ${_module}_IS_TEST AND
        NOT ${_module}_THIRD_PARTY)
        set(NO_PYTHON_BINDINGS_AVAILABLE TRUE)
        vtk_add_cs_wrapping(${_module})
        list(APPEND plugin_cs_modules ${_module})
    endif()
    unset(vtk-module)
  endforeach()

  # save the modules so any new plugins added, we can automatically make them
  # depend on these new modules.
  set (${pv-plugin}_CS_MODULES ${plugin_cs_modules})

  unset (VTK_MODULES_ALL)
  unset (current_module_set)
  unset (current_module_set_sorted)
  unset (plugin_cs_modules)
endmacro()

# this macro is used to setup the environment for loading/building VTK modules
# within ParaView plugins. This is only needed when building plugins outside of
# ParaVIew's source tree.
macro(pv_setup_module_environment _name)
  # Setup enviroment to build VTK modules outside of VTK source tree.
  set (BUILD_SHARED_LIBS ${VTK_BUILD_SHARED_LIBS})

  if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  endif()
  if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
  endif()
  if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
  endif()

  if (NOT VTK_INSTALL_RUNTIME_DIR)
    set(VTK_INSTALL_RUNTIME_DIR "bin")
  endif ()
  if (NOT VTK_INSTALL_LIBRARY_DIR)
    set(VTK_INSTALL_LIBRARY_DIR "lib/paraview-${PARAVIEW_VERSION}")
  endif ()
  if (NOT VTK_INSTALL_ARCHIVE_DIR)
    set(VTK_INSTALL_ARCHIVE_DIR "lib/paraview-${PARAVIEW_VERSION}")
  endif ()
  if (NOT VTK_INSTALL_INCLUDE_DIR)
    set(VTK_INSTALL_INCLUDE_DIR "include")
  endif ()
  if (NOT VTK_INSTALL_PACKAGE_DIR)
    set (VTK_INSTALL_PACKAGE_DIR "lib/cmake/${_name}")
  endif ()

  if (NOT VTK_FOUND)
    set (VTK_FOUND ${ParaView_FOUND})
  endif()
  if (VTK_FOUND)
    set (VTK_VERSION
      "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION}")
  endif()

  include(vtkExternalModuleMacros)
  include(vtkClientServerWrapping)
  if (PARAVIEW_ENABLE_PYTHON)
    include(vtkPythonWrapping)
  endif()

  # load information about existing modules.
  foreach (mod IN LISTS VTK_MODULES_ENABLED)
    vtk_module_load("${mod}")
  endforeach()

  # Set this so that we can track all the modules we're building for this
  # plugin. add_paraview_plugin() call will then add logic to automatically link
  # and (do CS init) for all modules that are built for the plugin. Note
  # pv_setup_module_environment() is not called for plugin being built as part
  # of the ParaView build, in that case pv-plugin is set when processing the
  # plugin.cmake file, and hence this logic still works!
  set (pv-plugin "${_name}")
endmacro()
