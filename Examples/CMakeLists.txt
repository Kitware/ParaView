
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

IF(NOT ParaView_BINARY_DIR)
  # we're building outside the ParaView tree.
  PROJECT(ParaViewExamples)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})

  SUBDIRS(Plugins)

ELSE(NOT ParaView_BINARY_DIR)

  # we're building inside the ParaView tree.

  # Build the examples as a separate project using a custom target.
  # Make sure it uses the same build configuration as ParaView.
  IF(CMAKE_CONFIGURATION_TYPES)
    SET(ParaViewExamples_CONFIG_TYPE -C "${CMAKE_CFG_INTDIR}")
  ELSE(CMAKE_CONFIGURATION_TYPES)
    SET(ParaViewExamples_CONFIG_TYPE)
  ENDIF(CMAKE_CONFIGURATION_TYPES)
  
  SET(PARAVIEW_TEST_COMMAND)
  IF(BUILD_TESTING)
    IF(APPLE)
      SET(PARAVIEW_TEST_COMMAND ${EXECUTABLE_OUTPUT_PATH}/paraview.app/Contents/MacOS/paraview 
                      --test-directory=${PARAVIEW_TEST_DIR})
    ELSE(APPLE)
      SET(PARAVIEW_TEST_COMMAND ${EXECUTABLE_OUTPUT_PATH}/paraview --test-directory=${PARAVIEW_TEST_DIR})
    ENDIF(APPLE)
  ENDIF(BUILD_TESTING)

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
                         -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
                         -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
                         -DCMAKE_C_FLAGS_DEBUG:STRING=${CMAKE_C_FLAGS_DEBUG}
                         -DCMAKE_C_FLAGS_RELEASE:STRING=${CMAKE_C_FLAGS_RELEASE}
                         -DCMAKE_C_FLAGS_MINSIZEREL:STRING=${CMAKE_C_FLAGS_MINSIZEREL}
                         -DCMAKE_C_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_C_FLAGS_RELWITHDEBINFO}
                         -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
                         -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
                         -DCMAKE_CXX_FLAGS_DEBUG:STRING=${CMAKE_CXX_FLAGS_DEBUG}
                         -DCMAKE_CXX_FLAGS_RELEASE:STRING=${CMAKE_CXX_FLAGS_RELEASE}
                         -DCMAKE_CXX_FLAGS_MINSIZEREL:STRING=${CMAKE_CXX_FLAGS_MINSIZEREL}
                         -DCMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
                         -DEXECUTABLE_OUTPUT_PATH:PATH=${EXECUTABLE_OUTPUT_PATH}
                         -DLIBRARY_OUTPUT_PATH:PATH=${LIBRARY_OUTPUT_PATH}
         )
  ADD_CUSTOM_TARGET(ParaViewExamplesTarget ALL DEPENDS
                    ${ParaView_BINARY_DIR}/ParaViewExamples)

  ADD_DEPENDENCIES(ParaViewExamplesTarget vtkPVFiltersCS)
  IF(PARAVIEW_BUILD_QT_GUI)
    ADD_DEPENDENCIES(ParaViewExamplesTarget pqComponents)
  ENDIF(PARAVIEW_BUILD_QT_GUI)

ENDIF(NOT ParaView_BINARY_DIR)

