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

  install(
    TARGETS     "${name}"
    DESTINATION bin
    COMPONENT   runtime
    EXPORT      ParaView)
endfunction ()
