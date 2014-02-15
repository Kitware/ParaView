find_package(OpenGL REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(quaternion_demo_SRCS  gpuhelper.cpp icosphere.cpp camera.cpp trackball.cpp quaternion_demo.cpp)

if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  set (Qt5_FIND_COMPONENTS
    OpenGL
    Gui
    )
  include (ParaViewQt5)
  qt5_automoc(${quaternion_demo_SRCS})
else ()
  find_package(Qt4 REQUIRED)
  set(QT_USE_QTOPENGL TRUE)
  include(${QT_USE_FILE})
  qt4_automoc(${quaternion_demo_SRCS})
  set(QT_LIBRARIES
    ${QT_LIBRARIES}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTOPENGL_LIBRARY}
    )
endif ()

add_executable(quaternion_demo ${quaternion_demo_SRCS})
add_dependencies(demos quaternion_demo)

target_link_libraries(quaternion_demo
  ${QT_LIBRARIES}
  ${OPENGL_LIBRARIES} )
