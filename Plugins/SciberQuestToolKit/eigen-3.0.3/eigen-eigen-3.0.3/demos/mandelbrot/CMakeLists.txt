if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  set (Qt5_FIND_COMPONENTS
    Core
    Gui
    )
  include (ParaViewQt5)
else ()
  find_package(Qt4 REQUIRED)
endif ()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (CMAKE_COMPILER_IS_GNUCXX)
   set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
   add_definitions ( "-DNDEBUG" )
endif (CMAKE_COMPILER_IS_GNUCXX)

if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  include_directories(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS})
else ()
  include_directories( ${QT_INCLUDE_DIR} )
endif ()

set(mandelbrot_SRCS
    mandelbrot.cpp
)

if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  qt5_automoc(${mandelbrot_SRCS})
else ()
  qt4_automoc(${mandelbrot_SRCS})
endif ()

add_executable(mandelbrot ${mandelbrot_SRCS})
add_dependencies(demos mandelbrot)

target_link_libraries(mandelbrot ${QT_LIBRARIES})
