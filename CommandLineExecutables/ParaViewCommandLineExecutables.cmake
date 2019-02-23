function (paraview_add_executable name)
  add_executable("${name}" ${ARGN})
  add_executable("ParaView::${name}" ALIAS "${name}")

  target_link_libraries("${name}"
    PRIVATE
      ParaView::ServerManagerApplication)

  if (NOT BUILD_SHARED_LIBS)
    target_link_libraries("${name}"
      PRIVATE
        paraview_plugins_static)
  endif ()

  if (PARAVIEW_ENABLE_PYTHON)
    target_link_libraries("${name}"
      PRIVATE
        ParaView::pvpythonmodules
        ParaView::PythonInitializer)
  endif ()

  if (paraview_exe_job_link_pool)
    set_property(TARGET "${name}"
      PROPERTY
        JOB_POOL_LINK "${paraview_exe_job_link_pool}")
  endif ()

  set(_paraview_launcher_RUNTIME_DESTINATION
    "${CMAKE_INSTALL_BINDIR}")
  set(_paraview_launcher_LIBRARY_DESTINATION
    "${CMAKE_INSTALL_LIBDIR}")

  install(
    TARGETS     "${name}"
    DESTINATION bin
    COMPONENT   runtime
    EXPORT      ParaView)
  
  # set up forwarding executables
  set(_paraview_launcher_destination "${_paraview_launcher_RUNTIME_DESTINATION}")
  if (BUILD_SHARED_LIBS AND UNIX AND NOT APPLE)
    set(_paraview_launcher_build_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set(_paraview_launcher_build_path "\"${_paraview_launcher_build_dir}\"")
    set(_paraview_launcher_install_dir "../${_paraview_launcher_LIBRARY_DESTINATION}")
    set(_paraview_launcher_install_path "\"${_paraview_launcher_install_dir}\"")
    set(_paraview_launcher_NAME ${name})
    set(_executable_cmake_dir "${ParaView_SOURCE_DIR}/CMake")
    configure_file(
      "${_executable_cmake_dir}/paraview_launcher.c.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${name}_launcher.c"
      @ONLY)
    add_executable("${name}-launcher"
      "${CMAKE_CURRENT_BINARY_DIR}/${name}_launcher.c")
    target_include_directories("${name}-launcher"
      PRIVATE
        "$<TARGET_PROPERTY:VTK::vtksys,INTERFACE_INCLUDE_DIRECTORIES>")
    set_target_properties("${name}-launcher"
      PROPERTIES
        OUTPUT_NAME               "${name}"
        RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_CURRENT_BINARY_DIR}/launcher")
    add_dependencies("${name}-launcher"
      "${name}")

    install(
      TARGETS "${name}-launcher"
      RUNTIME DESTINATION "${_paraview_launcher_RUNTIME_DESTINATION}")
    set(_paraview_launcher_destination "${_paraview_launcher_LIBRARY_DESTINATION}")
  endif()

  install(
    TARGETS "${name}"
    # TODO - need export targets?
    RUNTIME DESTINATION "${_paraview_launcher_destination}")
endfunction ()
