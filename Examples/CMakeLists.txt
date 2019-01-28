cmake_minimum_required(VERSION 3.8)
project(ParaViewExamples)

if (ParaView_BINARY_DIR)
  function (add_example dir)
    add_test(
      NAME    "ParaViewExample-${dir}"
      COMMAND "${CMAKE_CTEST_COMMAND}"
              --build-generator
                "${CMAKE_GENERATOR}"
              --build-and-test
                "${CMAKE_CURRENT_SOURCE_DIR}/${dir}"
                "${CMAKE_CURRENT_BINARY_DIR}/${dir}"
              --build-options
                "-DBUILD_TESTING=ON"
                "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
                "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
                "-DParaView_DIR=${paraview_cmake_build_dir}"
              --test-command
                "${CMAKE_CTEST_COMMAND}"
                  -C $<CONFIGURATION>)
    if (WIN32)
      set(example_path "$ENV{PATH};${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
      string(REPLACE ";" "\;" example_path "${example_path}")
      set_property(TEST "ParaViewExample-${dir}" APPEND
        PROPERTY
          ENVIRONMENT "PATH=${example_path}")
    endif ()
  endfunction ()
else ()
  find_package(ParaView REQUIRED)

  macro (add_example dir)
    add_subdirectory("${dir}")
  endmacro ()
endif ()

add_example(Plugins/Autostart)

if (FALSE)

add_subdirectory(Plugins/Filter)
add_subdirectory(Plugins/ParametricSource)
add_subdirectory(Plugins/SMProxy)
add_subdirectory(Plugins/Representation)

if (ENABLE_CATALYST)
  add_subdirectory(Catalyst)
endif ()

if (PARAVIEW_BUILD_QT_GUI)
  add_subdirectory(Plugins/DockWidget)
  add_subdirectory(Plugins/GUIToolBar)
  add_subdirectory(Plugins/LagrangianIntegrationModel)
  add_subdirectory(Plugins/OverrideXMLOnly)
  add_subdirectory(Plugins/PropertyWidgets)
  add_subdirectory(Plugins/Reader)
  add_subdirectory(Plugins/ReaderXMLOnly)
  # TODO: update this plugin to use the pipeline controller instead.
  #add_subdirectory(Plugins/RepresentationBehavior)
  add_subdirectory(Plugins/SourceToolbar)
  add_subdirectory(Plugins/Writer)

  add_subdirectory(CustomApplications/Clone1)
  add_subdirectory(CustomApplications/Clone2)
  add_subdirectory(CustomApplications/Demo0)
  add_subdirectory(CustomApplications/Demo1)
  add_subdirectory(CustomApplications/MultiServerClient)
# TODO Update this custom application to load only specific data types.
#  add_subdirectory(CustomApplications/Spreadsheet) 
endif()

if (TARGET vtkIOVisItBridge)
  # add_subdirectory(Plugins/VisItReader)
endif ()

endif ()
