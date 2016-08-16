if(PARAVIEW_BUILD_QT_GUI)
  include_directories(${VTK_INCLUDE_DIRS})
  include_directories(${PARAVIEW_GUI_INCLUDE_DIRS})
  set(srcs
    pqRemoteControl.cxx
    pqRemoteControlThread.cxx)

  pv_find_package_qt(qt_targets REQUIRED QUIET
    QT4_COMPONENTS QtGui QtNetwork
    QT5_COMPONENTS Widgets Network)

  pv_qt_wrap_cpp(MOC_SRCS pqRemoteControl.h pqRemoteControlThread.h)
  pv_qt_wrap_ui(UI_SRCS pqRemoteControl.ui)

  ADD_PARAVIEW_DOCK_WINDOW(
    OUTIFACES
    OUTSRCS
    CLASS_NAME pqRemoteControl
    DOCK_AREA Right)
  ADD_PARAVIEW_PLUGIN(MobileRemoteControl "1.0"
                      REQUIRED_ON_CLIENT
                      GUI_INTERFACES ${OUTIFACES}
                      GUI_SOURCES ${OUTSRCS} ${MOC_SRCS} ${UI_SRCS} ${srcs})
  target_link_libraries(MobileRemoteControl
                        LINK_PRIVATE
                          pqApplicationComponents
                          vtkPVClientServerCoreRendering
                          vtkPVServerManagerRendering
                          ${qt_targets})
endif()
