cmake_minimum_required(VERSION 3.8)
project(ParaViewExamples)

if (ParaView_BINARY_DIR)
  function (add_example dir)
    cmake_parse_arguments(arg "" "TEST_TIMEOUT" "" ${ARGN})
    if (arg_UNPARSED_ARGUMENTS)
      message(FATAL_ERROR "Unknown arguments passed to 'add_example': ${arg_UNPARSED_ARGUMENTS}")
    endif()

    add_test(
      NAME    "ParaViewExample-${dir}"
      COMMAND "${CMAKE_COMMAND}"
              "-Dconfig=$<CONFIGURATION>"
              "-Dgenerator=${CMAKE_GENERATOR}"
              "-Dsource=${CMAKE_CURRENT_SOURCE_DIR}"
              "-Dbinary=${CMAKE_CURRENT_BINARY_DIR}"
              "-Dexample_dir=${dir}"
              "-Dbuild_type=${CMAKE_BUILD_TYPE}"
              "-Dshared=${BUILD_SHARED_LIBS}"
              "-Dparaview_dir=${paraview_cmake_build_dir}"
              "-Dctest=${CMAKE_CTEST_COMMAND}"
              "-Dplatform=${CMAKE_GENERATOR_PLATFORM}"
              "-Dtoolset=${CMAKE_GENERATOR_TOOLSET}"
              "-Dparaview_binary_dir=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
              -P "${CMAKE_CURRENT_LIST_DIR}/RunExample.cmake")
    set_property(TEST "ParaViewExample-${dir}" APPEND
      PROPERTY
        SKIP_REGULAR_EXPRESSION "Skipping example")

    if (arg_TEST_TIMEOUT)
      # override timeout, if specified.
      set_property(TEST "ParaViewExample-${dir}"
        PROPERTY
          TIMEOUT "${arg_TEST_TIMEOUT}")
    endif()
  endfunction ()
else ()
  if (NOT BUILD_SHARED_LIBS)
    set(CMAKE_JOB_POOL_LINK static_exe_link)
    set_property(GLOBAL APPEND PROPERTY
      JOB_POOLS "${CMAKE_JOB_POOL_LINK}=1")
  endif ()

  find_package(ParaView REQUIRED)

  macro (add_example dir)
    add_subdirectory("${dir}")
  endmacro ()
endif ()

add_example(Plugins/AddPQProxy)
add_example(Plugins/Autostart)
add_example(Plugins/ComplexPluginArchitecture)
add_example(Plugins/ContextMenu)
add_example(Plugins/DockWidget)
if (TARGET ParaView::RemotingMisc)
  add_example(Plugins/ElevationFilter)
endif ()
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
add_example(CustomApplications/SimpleParaView)
# TODO Update this custom application to load only specific data types.
# add_example(CustomApplications/Spreadsheet)

# increate timeout for this test since it builds multiple executables
# which be timeconsuming on static builds.
add_example(Catalyst
  TEST_TIMEOUT 600)

add_example(Catalyst2/CFullExample)
add_example(Catalyst2/CxxFullExample)
add_example(Catalyst2/CxxImageDataExample)
add_example(Catalyst2/CxxMultiChannelInputExample)
