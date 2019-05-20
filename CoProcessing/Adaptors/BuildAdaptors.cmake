# Specific simulation adaptors are typically built separately from the ParaView
# build. The ParaView build will generally including the vtkPVCatalyst library,
# which is all anyone needs to write adaptors. However, to keep all
# open-adaptors in one place, we place them under this directory and build them
# using a pattern similar to the ParaView/Examples so that each one of the
# adaptors can be built separately if the user wants.
if (NOT PARAVIEW_ENABLE_CATALYST)
  # sanity check.
  return()
endif()

#------------------------------------------------------------------------------
# Make sure it uses the same build configuration as ParaView.
if (CMAKE_CONFIGURATION_TYPES)
  set(build_config_arg -C "${CMAKE_CFG_INTDIR}")
else()
  set(build_config_arg)
endif()

set (extra_params)
foreach (flag CMAKE_C_FLAGS_DEBUG
              CMAKE_C_FLAGS_RELEASE
              CMAKE_C_FLAGS_MINSIZEREL
              CMAKE_C_FLAGS_RELWITHDEBINFO
              CMAKE_CXX_FLAGS_DEBUG
              CMAKE_CXX_FLAGS_RELEASE
              CMAKE_CXX_FLAGS_MINSIZEREL
              CMAKE_CXX_FLAGS_RELWITHDEBINFO
              CMAKE_INSTALL_PREFIX)
  if (${${flag}})
    set (extra_params ${extra_params}
        -D${flag}:STRING=${${flag}})
  endif()
endforeach()

#------------------------------------------------------------------------------
set (SOURCE_DIR "${ParaView_SOURCE_DIR}/CoProcessing/Adaptors")
set (BINARY_DIR "${ParaView_BINARY_DIR}/CoProcessing/Adaptors")
make_directory("${BINARY_DIR}")

#------------------------------------------------------------------------------
# Function to easy adding separate custom-commands to build the adaptors.
#------------------------------------------------------------------------------
function(build_adaptor name languages)
  string(TOLOWER "${name}" lname)

  set(language_options)
  foreach (lang IN LISTS languages)
    list(APPEND language_options
      -DCMAKE_${lang}_COMPILER:FILEPATH=${CMAKE_${lang}_COMPILER}
      -DCMAKE_${lang}_FLAGS:STRING=${CMAKE_${lang}_FLAGS})
  endforeach ()

  #build-and-test source dir is buggy so we'll ensure it is known
  set(_source_dir_arg "-S")
  if(${CMAKE_VERSION} VERSION_LESS "3.13.0")
    #for older cmake, we have to use an undocumented flag to do this
    set(_source_dir_arg "-H")
  endif()

  #This generated file ensures that the adaptor's CMakeCache ends up with
  #the same CMAKE_PREFIX_PATH that ParaView's does, even if that has multiple
  #paths in it. It is necessary because ctest's argument parsing in the
  #custom command below destroys path separators.
  #Note: the generated file will become stale if these variables change.
  #In that case it will need manual intervention (remove it) to fix.
  file(GENERATE
    OUTPUT "${BINARY_DIR}/${lname}_build_options.cmake"
    CONTENT
"
set(ParaView_DIR ${ParaView_BINARY_DIR} CACHE PATH \"\")
set(QT_QMAKE_EXECUTABLE ${QT_QMAKE_EXECUTABLE} CACHE PATH \"\")
set(Qt5_DIR ${Qt5_DIR} CACHE PATH \"\")
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING \"\")
set(CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER} CACHE FILEPATH \"\")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING \"\")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} CACHE PATH \"\")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE PATH \"\")
set(CMAKE_PREFIX_PATH \"${CMAKE_PREFIX_PATH}\" CACHE STRING \"\")
"
)

  add_custom_command(
    OUTPUT "${BINARY_DIR}/${lname}.done"
    COMMAND ${CMAKE_CTEST_COMMAND}
            ${build_config_arg}
            --build-and-test ${SOURCE_DIR}/${name}
                             ${BINARY_DIR}/${name}
            --build-noclean
            --build-two-config
            --build-project ${name}
            --build-generator ${CMAKE_GENERATOR}
            --build-makeprogram ${CMAKE_MAKE_PROGRAM}
            --build-options -C "${BINARY_DIR}/${lname}_build_options.cmake"
                            ${language_options}
                            ${extra_params}
                            --no-warn-unused-cli
                           "${_source_dir_arg}${SOURCE_DIR}/${name}"
    COMMAND ${CMAKE_COMMAND}
            -E touch "${BINARY_DIR}/${lname}.done"

    ${ARGN}
  )
  add_custom_target(${name} ALL DEPENDS "${BINARY_DIR}/${lname}.done")
endfunction()


#------------------------------------------------------------------------------
# Adaptors
#------------------------------------------------------------------------------
cmake_dependent_option(BUILD_NPIC_ADAPTOR
  "Build the NPIC Catalyst Adaptor" OFF
  "PARAVIEW_BUILD_CATALYST_ADAPTORS" OFF)
mark_as_advanced(BUILD_NPIC_ADAPTOR)
if(BUILD_NPIC_ADAPTOR)
  build_adaptor(NPICAdaptor
    "C"
    COMMENT "Building NPIC Adaptor"
    DEPENDS vtkPVCatalyst)
endif()

if (PARAVIEW_USE_MPI)
  cmake_dependent_option(BUILD_PARTICLE_ADAPTOR
    "Build the Particle Catalyst Adaptor" OFF
    "PARAVIEW_BUILD_CATALYST_ADAPTORS" OFF)
  mark_as_advanced(BUILD_PARTICLE_ADAPTOR)
  if(BUILD_PARTICLE_ADAPTOR)
    build_adaptor(ParticleAdaptor
      "C"
      COMMENT "Building Particle Adaptor"
      DEPENDS vtkPVCatalyst)
  endif()
endif()

#------------------------------------------------------------------------------
# Adaptors that need Fortran -- we disable them by default because not all
# systems load all of the proper Fortran dependencies like MPI_Fortran_LIBRARIES
#------------------------------------------------------------------------------
if (CMAKE_Fortran_COMPILER_WORKS)
  cmake_dependent_option(BUILD_PHASTA_ADAPTOR
    "Build the Phasta Catalyst Adaptor" OFF
    "PARAVIEW_BUILD_CATALYST_ADAPTORS" OFF)
  mark_as_advanced(BUILD_PHASTA_ADAPTOR)
  if(BUILD_PHASTA_ADAPTOR)
    build_adaptor(PhastaAdaptor
      "C;Fortran"
      COMMENT "Building Phasta Adaptor"
      DEPENDS vtkPVCatalyst)
  endif()
endif()

#------------------------------------------------------------------------------
# Adaptors that need Python
#------------------------------------------------------------------------------
if (PARAVIEW_ENABLE_PYTHON AND NOT WIN32)
  # Add CTHAdaptor if Python is enabled.
  build_adaptor(CTHAdaptor
    "C"
    COMMENT "Building CTH Adaptor"
    DEPENDS vtkPVPythonCatalyst)

  if (PARAVIEW_USE_MPI)
      build_adaptor(CamAdaptor
                    ""
                    COMMENT "Building Cam Adaptor"
                    DEPENDS vtkPVCatalyst)
  endif()

  #------------------------------------------------------------------------------
  # Adaptors that need Python and Fortran
  # The Pagosa adaptor is done as part of the normal ParaView CMake configuration
  # so that the library can be installed.
  #------------------------------------------------------------------------------
  cmake_dependent_option(PARAVIEW_BUILD_PAGOSA_ADAPTOR
    "Build the Pagosa Catalyst Adaptor" OFF
    "PARAVIEW_BUILD_CATALYST_ADAPTORS" OFF)
  mark_as_advanced(PARAVIEW_BUILD_PAGOSA_ADAPTOR)
  if(PARAVIEW_BUILD_PAGOSA_ADAPTOR)
    add_subdirectory(CoProcessing/Adaptors/PagosaAdaptor)
  endif()

endif()
