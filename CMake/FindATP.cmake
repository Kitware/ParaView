# Find the Cray ATP library
# defines:
#   ATP_FOUND - System has ATP
#   ATP_LIBRARIES - The libraries needed to use ATP
#   ATP_LINK_FLAGS - Required link flags
# options:
#   if FindATP_DEBUG is set then information about
#   what was found is printed.
if (NOT ATP_FOUND)
  find_package(PkgConfig)
  pkg_check_modules(ATP_TMP QUIET AtpSigHandler)

  # convert naked libs into libs with full paths
  set(ATP_TMP_LIBS)
  foreach(tmp_lib ${ATP_TMP_LIBRARIES})
    set(${tmp_lib}_lib)
    find_library(
      ${tmp_lib}_lib NAMES ${tmp_lib}
      HINTS ${ATP_TMP_LIBRARY_DIRS})
    if (${tmp_lib}_lib)
      set(ATP_TMP_LIBS ${ATP_TMP_LIBS} ${${tmp_lib}_lib})
    endif()
  endforeach()

  # get rid of ; which break the link command
  set(ATP_TMP_FLAGS)
  foreach(tmp_flag ${ATP_TMP_LDFLAGS_OTHER})
    set(ATP_TMP_FLAGS "${ATP_TMP_FLAGS} ${tmp_flag}")
  endforeach()

  set(ATP_LIBRARIES ${ATP_TMP_LIBS} CACHE STRING "Cray ATP link libraries")
  set(ATP_LINK_FLAGS ${ATP_TMP_FLAGS} CACHE STRING "Cray ATP extra compiler flags")
  mark_as_advanced(ATP_LIBRARIES ATP_LINK_FLAGS)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ATP DEFAULT_MSG ATP_LIBRARIES)

  if (FindATP_DEBUG)
    message("ATP_TMP_FOUND=${ATP_TMP_FOUND}")
    message("ATP_TMP_LIBRARIES=${ATP_TMP_LIBRARIES}")
    message("ATP_TMP_LIBRARY_DIRS=${ATP_TMP_LIBRARY_DIRS}")
    message("ATP_TMP_LDFLAGS=${ATP_TMP_LDFLAGS}")
    message("ATP_TMP_LDFLAGS_OTHER=${ATP_TMP_LDFLAGS_OTHER}")
    message("ATP_TMP_INCLUDE_DIRS=${ATP_TMP_INCLUDE_DIRS}")
    message("ATP_TMP_CFLAGS=${ATP_TMP_CFLAGS}")
    message("ATP_TMP_CFLAGS_OTHER=${ATP_TMP_CFLAGS_OTHER}")
    message("ATP_LIBRARIES=${ATP_LIBRARIES}")
    message("ATP_LINK_FLAGS=${ATP_LINK_FLAGS}")
  endif()
endif()
