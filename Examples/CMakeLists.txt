cmake_minimum_required(VERSION 3.8)
project(ParaViewExamples)

if (CMAKE_VERSION VERSION_LESS "3.11" AND
    CMAKE_GENERATOR STREQUAL "Ninja")
  message(WARNING
    "CMake < 3.11 with the Ninja generator causes issues with the way "
    "examples are built. Skipping examples for now.")
  return ()
endif ()

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
                # XXX(paraview-config): `find_package(ParaView)` should provide
                # this information automatically. In this case, VTK should be
                # providing this information.
                # https://gitlab.kitware.com/vtk/vtk/issues/17503
                "-DQt5_DIR=${Qt5_DIR}"
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
add_example(Plugins/DockWidget)
add_example(Plugins/ElevationFilter)
add_example(Plugins/GUIMyToolBar)
add_example(Plugins/LagrangianIntegrationModel)
add_example(Plugins/MyPNGReader)
add_example(Plugins/MyTiffWriter)
add_example(Plugins/OverrideXMLOnly)
add_example(Plugins/PropertyWidgets)
add_example(Plugins/ReaderXMLOnly)
add_example(Plugins/Representation)
# TODO: update this plugin to use the pipeline controller instead.
#add_example(Plugins/RepresentationBehavior)
add_example(Plugins/SMParametricSource)
add_example(Plugins/SMMyProxy)
add_example(Plugins/SourceToolbar)
# add_example(Plugins/VisItReader)

add_example(CustomApplications/Clone1)
add_example(CustomApplications/Clone2)
add_example(CustomApplications/Demo0)
add_example(CustomApplications/Demo1)
add_example(CustomApplications/MultiServerClient)
# TODO Update this custom application to load only specific data types.
# add_example(CustomApplications/Spreadsheet)

add_example(Catalyst)
