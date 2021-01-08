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
