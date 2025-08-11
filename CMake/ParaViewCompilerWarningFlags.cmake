# Silence spurious -Wattribute warnings on GCC < 9.1:
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89325
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
  CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
  target_compile_options(paraviewbuild
    INTERFACE
      "$<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:C>:-Wno-attributes>>"
      "$<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:CXX>:-Wno-attributes>>")
endif ()

# This module requires CMake 3.19 features (the `CheckCompilerFlag`
# module). Just skip it for older CMake versions.
if (CMAKE_VERSION VERSION_LESS "3.19")
  return()
endif ()

include(CheckCompilerFlag)

function(paraview_add_flag flag)
  foreach (lang IN LISTS ARGN)
    check_compiler_flag("${lang}" "${flag}" "paraview_have_compiler_flag-${lang}-${flag}")
    if (paraview_have_compiler_flag-${lang}-${flag})
      target_compile_options(paraviewbuild
        INTERFACE
        "$<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:${lang}>:${flag}>>")
    endif ()
  endforeach ()
endfunction()

option(PARAVIEW_ENABLE_EXTRA_BUILD_WARNINGS "Enable extra build warnings" OFF)
mark_as_advanced(PARAVIEW_ENABLE_EXTRA_BUILD_WARNINGS)

if (PARAVIEW_ENABLE_EXTRA_BUILD_WARNINGS)
  # C and C++ flags.
  set(langs C CXX)
  # Ignored warnings. Should be investigated and false positives reported to
  # GCC and actual bugs fixed.
  paraview_add_flag(-Wno-stringop-overflow ${langs}) # VTK issue 19306
  paraview_add_flag(-Wno-stringop-overread ${langs}) # VTK issue 19307
endif ()
