if (CMAKE_COMPILER_IS_GNUCXX)

  include(CheckCXXCompilerFlag)

  # Additional warnings for GCC
  set(paraview_extra_warning_flags
    -Wnon-virtual-dtor
    -Wno-long-long
    -ansi
    -Wcast-align
    -Wchar-subscripts
    -Wall
    -Wextra
    -Wpointer-arith
    -Wformat-security
    -Woverloaded-virtual
    -Wshadow
    -Wunused-parameter
    -fno-check-new
    -Werror=undef)

  # This flag is useful as not returning from a non-void function is an error
  # with MSVC, but it is not supported on all GCC compiler versions
  check_cxx_compiler_flag(-Werror=return-type HAVE_GCC_ERROR_RETURN_TYPE)
  if (HAVE_GCC_ERROR_RETURN_TYPE)
    list(APPEND paraview_extra_warning_flags
      -Werror=return-type)
  endif ()

  # If we are compiling on Linux then set some extra linker flags too
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    option(PARAVIEW_LINKER_FATAL_WARNINGS "Specify if linker warnings must be considered as errors." OFF)
    mark_as_advanced(PARAVIEW_LINKER_FATAL_WARNINGS)
    if (TARGET paraviewbuild)
      # XXX(cmake-3.13): use `target_link_options`
      set_property(TARGET paraviewbuild APPEND
        PROPERTY
          INTERFACE_LINK_OPTIONS
            "$<$<BOOL:${PARAVIEW_LINKER_FATAL_WARNINGS}>:LINKER:--fatal-warnings>")
    endif ()
  endif ()

  # Set up the debug CXX_FLAGS for extra warnings
  option(PARAVIEW_EXTRA_COMPILER_WARNINGS
    "Add compiler flags to do stricter checking" OFF)
  if (PARAVIEW_EXTRA_COMPILER_WARNINGS)
    if (TARGET paraviewbuild)
      target_compile_options(paraviewbuild
        INTERFACE
          "$<$<COMPILE_LANGUAGE:CXX>${paraview_extra_warning_flags}>")
    endif ()
  endif ()

  # Silence spurious -Wattribute warnings on GCC < 9.1:
  # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=89325
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
    target_compile_options(paraviewbuild
      INTERFACE
        # XXX(cmake-3.15): `COMPILE_LANGUAGE` supports multiple languages.
        "$<BUILD_INTERFACE:$<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-Wno-attributes>>")
  endif ()
endif ()

# Intel OneAPI compilers >= 2021.2.0 turn on "fast math" at any non-zero
# optimization level. Suppress this non-standard behavior using the
# `-fp-model=precise` flag.
set(intel_oneapi_compiler_detections)
set(intel_oneapi_compiler_version_min "2021.2.0")
foreach (lang IN ITEMS C CXX Fortran)
  if (CMAKE_VERSION VERSION_LESS "3.14" AND lang STREQUAL "Fortran") # XXX(cmake-3.14): `Fortran_COMPILER_ID` genex
    continue ()
  endif ()
  # Detect the IntelLLVM compiler for the given language.
  set(is_lang "$<COMPILE_LANGUAGE:${lang}>")
  set(is_intelllvm "$<${lang}_COMPILER_ID:IntelLLVM>")
  set(is_intelllvm_fastmath_assuming_version "$<VERSION_GREATER_EQUAL:$<${lang}_COMPILER_VERSION>,${intel_oneapi_compiler_version_min}>")
  list(APPEND intel_oneapi_compiler_detections
    "$<AND:${is_lang},${is_intelllvm},${is_intelllvm_fastmath_assuming_version}>")
endforeach ()
string(REPLACE ";" "," intel_oneapi_compiler_detections "${intel_oneapi_compiler_detections}")
if (TARGET paraviewbuild)
  target_compile_options(paraviewbuild
    INTERFACE
      "$<BUILD_INTERFACE:$<$<OR:${intel_oneapi_compiler_detections}>:-fp-model=precise>>")
endif ()
