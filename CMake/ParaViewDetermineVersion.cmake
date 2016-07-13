#=========================================================================
#
#  Program:   ParaView
#
#  Copyright (c) Kitware, Inc.
#  All rights reserved.
#  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
#
#     This software is distributed WITHOUT ANY WARRANTY; without even
#     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#     PURPOSE.  See the above copyright notice for more information.
#
#=========================================================================

# Used to determine the version for ParaView source using "git describe", if git
# is found. On success sets following variables in caller's scope:
#   ${var_prefix}_VERSION
#   ${var_prefix}_VERSION_MAJOR
#   ${var_prefix}_VERSION_MINOR
#   ${var_prefix}_VERSION_PATCH
#   ${var_prefix}_VERSION_PATCH_EXTRA
#   ${var_prefix}_VERSION_FULL
#   ${var_prefix}_VERSION_IS_RELEASE is true, if patch-extra is empty.
#
# If git is not found, or git describe cannot be run successfully, then these
# variables are left unchanged and status message is printed.
#
# Arguments are:
#   source_dir : Source directory
#   git_command : git executable
#   var_prefix : prefix for variables e.g. "PARAVIEW".
function(determine_version source_dir git_command var_prefix)
  if ("$Format:$" STREQUAL "")
    # We are in an exported tarball and should use the shipped version
    # information. Just return here to avoid the warning message at the end of
    # this function.
    return ()
  elseif (NOT PARAVIEW_GIT_DESCRIBE)
    if(EXISTS ${git_command})
      execute_process(
        COMMAND ${git_command} describe
        WORKING_DIRECTORY ${source_dir}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE)
    endif()
  else()
    set(result 0)
    set(output ${PARAVIEW_GIT_DESCRIBE})
  endif()
  extract_version_components("${output}" tmp)
  if(DEFINED tmp_VERSION)
    message(STATUS "Determined Source Version : ${tmp_VERSION_FULL}")
    foreach(suffix VERSION VERSION_MAJOR VERSION_MINOR VERSION_PATCH
                   VERSION_PATCH_EXTRA VERSION_FULL VERSION_IS_RELEASE)
      set(${var_prefix}_${suffix} ${tmp_${suffix}} PARENT_SCOPE)
    endforeach()
  else()
    message(STATUS
      "Could not use git to determine source version, using version ${${var_prefix}_VERSION_FULL}")
  endif()
endfunction()

# Extracts components from a version string. See determine_version() for usage.
function(extract_version_components version_string var_prefix)
  string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)[-]*(.*)"
    version_matches "${version_string}")
  if(CMAKE_MATCH_0)
    set(full ${CMAKE_MATCH_0})
    set(major ${CMAKE_MATCH_1})
    set(minor ${CMAKE_MATCH_2})
    set(patch ${CMAKE_MATCH_3})
    set(patch_extra ${CMAKE_MATCH_4})

    set(${var_prefix}_VERSION "${major}.${minor}" PARENT_SCOPE)
    set(${var_prefix}_VERSION_MAJOR ${major} PARENT_SCOPE)
    set(${var_prefix}_VERSION_MINOR ${minor} PARENT_SCOPE)
    set(${var_prefix}_VERSION_PATCH ${patch} PARENT_SCOPE)
    set(${var_prefix}_VERSION_PATCH_EXTRA ${patch_extra} PARENT_SCOPE)
    set(${var_prefix}_VERSION_FULL ${full} PARENT_SCOPE)
    if("${major}.${minor}.${patch}" VERSION_EQUAL "${full}")
      set(${var_prefix}_VERSION_IS_RELEASE TRUE PARENT_SCOPE)
    else()
      set(${var_prefix}_VERSION_IS_RELEASE FALSE PARENT_SCOPE)
    endif()
  endif()
endfunction()
