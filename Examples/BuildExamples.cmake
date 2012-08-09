# Build the examples as a separate project using a custom target.
# Make sure it uses the same build configuration as ParaView.
if (CMAKE_CONFIGURATION_TYPES)
  set(ParaViewExamples_CONFIG_TYPE -C "${CMAKE_CFG_INTDIR}")
else()
  set(ParaViewExamples_CONFIG_TYPE)
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


ADD_CUSTOM_COMMAND(
  OUTPUT ${ParaView_BINARY_DIR}/ParaViewExamples
  COMMAND ${CMAKE_CTEST_COMMAND}
  ARGS ${ParaViewExamples_CONFIG_TYPE}
       --build-and-test
       ${ParaView_SOURCE_DIR}/Examples
       ${ParaView_BINARY_DIR}/Examples/All
       --build-noclean
       --build-two-config
       --build-project ParaViewExamples
       --build-generator ${CMAKE_GENERATOR}
       --build-makeprogram ${CMAKE_MAKE_PROGRAM}
       --build-options -DParaView_DIR:PATH=${ParaView_BINARY_DIR}
                       -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                       -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                       -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
                       -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
                       -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
                       ${extra_params}
                       -DEXECUTABLE_OUTPUT_PATH:PATH=${EXECUTABLE_OUTPUT_PATH}
                       -DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
       )
ADD_CUSTOM_TARGET(ParaViewExamplesTarget ALL DEPENDS
                  ${ParaView_BINARY_DIR}/ParaViewExamples)

add_dependencies(ParaViewExamplesTarget vtkPVServerManagerApplication)
IF(PARAVIEW_BUILD_QT_GUI)
  add_dependencies(ParaViewExamplesTarget pqApplicationComponents)
ENDIF(PARAVIEW_BUILD_QT_GUI)
