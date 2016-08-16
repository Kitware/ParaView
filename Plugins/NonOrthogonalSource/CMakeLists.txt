SET(PLUGIN_NAME NonOrthogonalSource)
SET(PLUGIN_VERSION "1.0")

include_directories(${VTK_INCLUDE_DIRS})

set(sources)

if(PARAVIEW_BUILD_QT_GUI)
  pv_find_package_qt(qt_targets REQUIRED QUIET
    QT4_COMPONENTS QtGui
    QT5_COMPONENTS Widgets)
  pv_qt_wrap_cpp(MOC_SRCS pqNonOrthogonalAutoStart.h)
  add_paraview_auto_start(IFACES IFACE_SRCS
    CLASS_NAME pqNonOrthogonalAutoStart
    STARTUP startup
    SHUTDOWN shutdown)
  list(APPEND sources
    pqNonOrthogonalAutoStart.cxx
    ${MOC_SRCS}
    ${IFACE_SRCS})
endif()

add_paraview_plugin(NonOrthogonalSource "1.0"
  SERVER_MANAGER_XML
    vtkNonOrthogonalSource.xml
  SERVER_MANAGER_SOURCES
    vtkShearedWaveletSource.cxx
  GUI_INTERFACES ${IFACES}
  SOURCES ${sources}
)
if (PARAVIEW_BUILD_QT_GUI)
  target_link_libraries(NonOrthogonalSource LINK_PRIVATE pqApplicationComponents)
endif()

if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
