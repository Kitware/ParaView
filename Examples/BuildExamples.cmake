#------------------------------------------------------------------------------
# Builds the examples as a separate project using a custom target.
# This is included in ParaView/CMakeLists.txt to build examples as a separate
# project.

#------------------------------------------------------------------------------
# Make sure it uses the same build configuration as ParaView.
if (CMAKE_CONFIGURATION_TYPES)
  set(build_config_arg -C "${CMAKE_CFG_INTDIR}")
else()
  set(build_config_arg)
endif()

set (extra_params)
foreach (flag CMAKE_C_FLAGS_DEBUG
              CMAKE_C_FLAGS_RELEASE
              CMAKE_C_FLAGS_MINSIZEREL
              CMAKE_C_FLAGS_RELWITHDEBINFO
              CMAKE_CXX_FLAGS_DEBUG
              CMAKE_CXX_FLAGS_RELEASE
              CMAKE_CXX_FLAGS_MINSIZEREL
              CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  if (${${flag}})
    set (extra_params ${extra_params}
        -D${flag}:STRING=${${flag}})
  endif()
endforeach()

set (examples_dependencies
  vtkPVServerManagerApplication
  vtkPVServerManagerApplicationCS)
if (PARAVIEW_BUILD_QT_GUI)
  list (APPEND examples_dependencies pqApplicationComponents)
endif()

set(ENABLE_CATALYST OFF)
if (PARAVIEW_ENABLE_PYTHON AND PARAVIEW_USE_MPI AND PARAVIEW_ENABLE_CATALYST AND NOT WIN32)
  list (APPEND examples_dependencies vtkPVPythonCatalyst)
  set (ENABLE_CATALYST ON)
endif()

add_custom_command(
  OUTPUT "${ParaView_BINARY_DIR}/ParaViewExamples.done"
  COMMAND ${CMAKE_CTEST_COMMAND}
       ${build_config_arg}
       --build-and-test
       ${ParaView_SOURCE_DIR}/Examples
       ${ParaView_BINARY_DIR}/Examples/All
       --build-noclean
       --build-two-config
       --build-project ParaViewExamples
       --build-generator ${CMAKE_GENERATOR}
       --build-makeprogram ${CMAKE_MAKE_PROGRAM}
       --build-options -DParaView_DIR:PATH=${ParaView_BINARY_DIR}
                       -DPARAVIEW_QT_VERSION:STRING=${PARAVIEW_QT_VERSION}
                       -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
                       -DQt5_DIR:PATH=${Qt5_DIR}
                       -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                       -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                       -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
                       -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
                       -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
                       -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                       -DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                       -DBUILD_TESTING:BOOL=${BUILD_TESTING}
                       -DPARAVIEW_TEST_OUTPUT_DIR:PATH=${PARAVIEW_TEST_OUTPUT_DIR}
                       -DENABLE_CATALYST:BOOL=${ENABLE_CATALYST}
                       ${extra_params}
                       --no-warn-unused-cli
  COMMAND ${CMAKE_COMMAND} -E touch
          "${ParaView_BINARY_DIR}/ParaViewExamples.done"
  COMMENT "Build examples as a separate project"
  DEPENDS ${examples_dependencies}
)
# Add custom target to ensure that the examples get built.
add_custom_target(ParaViewExamples ALL DEPENDS
  "${ParaView_BINARY_DIR}/ParaViewExamples.done")
