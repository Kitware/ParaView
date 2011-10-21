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

# Used to determine the version for ParaView source using "git describe".
# source_dir : Source directory
# git_command : git executable
# alternative_version_file : file to read if git fails.
# var_prefix : prefix for variables e.g. "PARAVIEW".
function(determine_version source_dir git_command alternative_version_file var_prefix)
  set (major)
  set (minor)
  set (patch)
  set (full)
  execute_process(
    COMMAND ${git_command} describe
    WORKING_DIRECTORY ${source_dir}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE)
  if (${result} EQUAL 0)
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)[-]*(.*)"
      version_matches ${output})
    if (CMAKE_MATCH_0) 
      message(STATUS "Determined Source Version : ${CMAKE_MATCH_0}")
      set (full ${CMAKE_MATCH_0})
      set (major ${CMAKE_MATCH_1})
      set (minor ${CMAKE_MATCH_2})
      set (patch ${CMAKE_MATCH_3})
      set (patch_extra ${CMAKE_MATCH_4})
      # not sure if I want to write the file out yet.
      #file (WRITE ${alternative_version_file} ${full})
    endif()
  endif()

  if (NOT major)
    # Check if file exists, use that.
    if(EXISTS "${alternative_version_file}" AND NOT IS_DIRECTORY "${alternative_version_file}")
      file (STRINGS ${alternative_version_file} contents)
      string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)[-]*(.*)"
        version_matches ${contents})
      if (CMAKE_MATCH_0) 
        message(STATUS "Determined Source Version from file : ${CMAKE_MATCH_0}")
        set (full ${CMAKE_MATCH_0})
        set (major ${CMAKE_MATCH_1})
        set (minor ${CMAKE_MATCH_2})
        set (patch ${CMAKE_MATCH_3})
        set (patch_extra ${CMAKE_MATCH_4})
      endif()
    else(EXISTS "${alternative_version_file}" AND NOT IS_DIRECTORY "${alternative_version_file}")
      message (STATUS "Could NOT find ${alternative_version_file} which is mandatory if Git is not found either.")
    endif()
  endif()

  if (NOT major)
    message (FATAL_ERROR "Failed to determine source version correctly.")
  endif()

  set (${var_prefix}_VERSION "${major}.${minor}" PARENT_SCOPE)
  set (${var_prefix}_VERSION_MAJOR ${major} PARENT_SCOPE)
  set (${var_prefix}_VERSION_MINOR ${minor} PARENT_SCOPE)
  set (${var_prefix}_VERSION_PATCH ${patch} PARENT_SCOPE)
  set (${var_prefix}_VERSION_PATCH_EXTRA ${patch_extra} PARENT_SCOPE)
  set (${var_prefix}_VERSION_FULL ${full} PARENT_SCOPE)
  if ("${major}.${minor}.${patch}" EQUAL "${full}")
    set (${var_prefix}_VERSION_IS_RELEASE TRUE PARENT_SCOPE)
  else ()
    set (${var_prefix}_VERSION_IS_RELEASE FALSE PARENT_SCOPE)
  endif()
endfunction() 
