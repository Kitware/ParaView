cmake_minimum_required(VERSION 3.3)

if (NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

include(ParaViewPlugins)

include_directories(
  ${VTK_INCLUDE_DIRS}
)

if (PARAVIEW_BUILD_QT_GUI)
  pv_find_package_qt(qt_targets REQUIRED QUIET
    QT5_COMPONENTS Widgets)

  pv_qt_wrap_cpp(mocsrcs
    pqExtrusionPropertyWidgetDecorator.h)

  set(outifaces)
  set(outsrcs)
  add_paraview_property_widget_decorator(outifaces outsrcs
    TYPE "ExtrusionPropertyWidgetDecorator"
    CLASS_NAME pqExtrusionPropertyWidgetDecorator)
  set(qtsrcs pqExtrusionPropertyWidgetDecorator.cxx)
endif()

encode_files_as_strings(shader_srcs
  shaders/vtkExtrudeCell_gs.glsl)

add_paraview_plugin(EmbossingRepresentations "1.0"
  SERVER_MANAGER_XML
    EmbossingRepresentations.xml
  SERVER_MANAGER_SOURCES
    vtkBumpMapRepresentation.cxx
    vtkExtrusionRepresentation.cxx
  GUI_INTERFACES
    ${outifaces}
  SOURCES
    ${outsrcs}
    ${mocsrcs}
    ${qtsrcs}
    vtkBumpMapMapper.cxx
    vtkExtrusionMapper.cxx
    ${shader_srcs})

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
