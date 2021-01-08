function (paraview_migrate_setting oldname newname doc)
  if (DEFINED "${oldname}" AND
      NOT DEFINED "${newname}")
    set("${newname}" "${${oldname}}" CACHE STRING "${doc}")
    unset("${oldname}")
    unset("${oldname}" CACHE)
    message(AUTHOR_WARNING
      "The `${oldname}` setting has been migrated to `${newname}`.")
  endif ()
endfunction ()

# XXX(paraview-5.10): Once there is a version number that users can use to
# determine what is needed, migrate ParaView::cgns settings for VTK::cgns.
paraview_migrate_setting(
  VTK_MODULE_ENABLE_ParaView_cgns
  VTK_MODULE_ENABLE_VTK_cgns
  "Migration of ParaView::cgns settings")
paraview_migrate_setting(
  VTK_MODULE_USE_EXTERNAL_ParaView_cgns
  VTK_MODULE_USE_EXTERNAL_VTK_cgns
  "Migration of ParaView::cgns settings")
paraview_migrate_setting(
  VTK_MODULE_ENABLE_ParaView_VTKExtensionsCGNSReader
  VTK_MODULE_ENABLE_VTK_IOCGNSReader
  "Migration of ParaView::VTKExtensionsCGNSReader settings")
