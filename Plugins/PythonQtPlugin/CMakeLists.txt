if (ParaView_SOURCE_DIR)
else()
  cmake_minimum_required(VERSION 2.8)
  project(PythonQtPlugin)

  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
find_package(PythonQt REQUIRED)

include_directories(${PYTHON_INCLUDE_PATH})
include_directories(${PYTHONQT_INCLUDE_DIR})
include_directories(${VTK_INCLUDE_DIRS})

set(decorator_file ${CMAKE_CURRENT_BINARY_DIR}/pqPluginDecorators.h)

set(PythonQtPlugin_MOC_SRCS
  pqPythonQtPlugin.h
  pqPythonQtMethodHelpers.h
  pqPythonEventFilter.h
  )

if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  qt5_wrap_cpp(MOC_SRCS
    ${PythonQtPlugin_MOC_SRCS}
    ${decorator_file}
    )
else ()
  qt4_wrap_cpp(MOC_SRCS
    ${PythonQtPlugin_MOC_SRCS}
    ${decorator_file}
    )
endif ()

set(srcs
  pqPythonQtPlugin.cxx
  pqPythonQtWrapperFactory.cxx
  )

set(deps
  pqApplicationComponents
  vtkWrappingPythonCore
  ${PYTHONQT_LIBRARIES}
  ${PYTHON_LIBRARIES}
  )

add_paraview_auto_start(
  OUTIFACES
  OUTSRCS
  CLASS_NAME pqPythonQtPlugin)


add_paraview_plugin(PythonQtPlugin "1.0"
                    GUI_INTERFACES ${OUTIFACES}
                    GUI_SOURCES ${OUTSRCS} ${MOC_SRCS} ${srcs}
                    REQUIRED_ON_CLIENT)

target_link_libraries(PythonQtPlugin LINK_PRIVATE ${deps})

find_package(PythonInterp REQUIRED)
add_custom_command(OUTPUT ${decorator_file}
  COMMAND ${PYTHON_EXECUTABLE} WrapPythonQt.py
  -i wrapped_methods.txt -o "${decorator_file}"
  --class-prefixes vtk pq --qt-class-prefixes pq
  --module-name paraview
  DEPENDS wrapped_methods.txt WrapPythonQt.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
