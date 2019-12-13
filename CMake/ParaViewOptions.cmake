#========================================================================
# BUILD OPTIONS:
# Options that affect the ParaView build, in general.
# These should begin with `PARAVIEW_BUILD_`.
#========================================================================

vtk_deprecated_setting(shared_default PARAVIEW_BUILD_SHARED_LIBS BUILD_SHARED_LIBS "ON")
option(PARAVIEW_BUILD_SHARED_LIBS "Build ParaView with shared libraries" "${shared_default}")

vtk_deprecated_setting(legacy_remove_default PARAVIEW_BUILD_LEGACY_REMOVE VTK_LEGACY_REMOVE "OFF")
option(PARAVIEW_BUILD_LEGACY_REMOVE "Remove all legacy code completely" ${legacy_remove_default})
mark_as_advanced(PARAVIEW_BUILD_LEGACY_REMOVE)

vtk_deprecated_setting(legacy_silent_default PARAVIEW_BUILD_LEGACY_SILENT VTK_LEGACY_SILENT "OFF")
option(PARAVIEW_BUILD_LEGACY_SILENT "Silence all legacy code messages" ${legacy_silent_default})
mark_as_advanced(PARAVIEW_BUILD_LEGACY_SILENT)

# Kits bundle multiple modules together into a single library, this
# is used to dramatically reduce the number of generated libraries.
vtk_deprecated_setting(kits_default PARAVIEW_BUILD_WITH_KITS PARAVIEW_ENABLE_KITS "OFF")
option(PARAVIEW_BUILD_WITH_KITS "Build ParaView using kits instead of modules." OFF)
mark_as_advanced(PARAVIEW_BUILD_WITH_KITS)

vtk_deprecated_setting(external_default PARAVIEW_BUILD_WITH_EXTERNAL PARAVIEW_USE_EXTERNAL "OFF")
option(PARAVIEW_BUILD_WITH_EXTERNAL "Use external copies of third party libraries by default" OFF)
mark_as_advanced(PARAVIEW_BUILD_WITH_EXTERNAL)

option(PARAVIEW_BUILD_ALL_MODULES "Build all modules by default" OFF)
mark_as_advanced(PARAVIEW_BUILD_ALL_MODULES)

option(PARAVIEW_BUILD_EXAMPLES "Enable ParaView examples" OFF)
set(PARAVIEW_BUILD_TESTING "OFF"
  CACHE STRING "Enable testing")
set_property(CACHE PARAVIEW_BUILD_TESTING
  PROPERTY
    STRINGS "ON;OFF;WANT")

cmake_dependent_option(PARAVIEW_BUILD_VTK_TESTING "Enable VTK testing" OFF
  "PARAVIEW_BUILD_TESTING" OFF)

option(PARAVIEW_BUILD_DEVELOPER_DOCUMENTATION "Generate ParaView C++/Python docs" ${doc_default})

#========================================================================
# CAPABILITY OPTIONS:
# Options that affect the build capabilities.
# These should begin with `PARAVIEW_USE_`.
#========================================================================

# XXX(VTK): External VTK is not yet actually supported.
if (FALSE)
option(PARAVIEW_USE_EXTERNAL_VTK "Use an external VTK." OFF)
mark_as_advanced(PARAVIEW_USE_EXTERNAL_VTK)
else ()
set(PARAVIEW_USE_EXTERNAL_VTK OFF)
endif ()

option(PARAVIEW_USE_MPI "Enable MPI support for parallel computing" OFF)
option(PARAVIEW_USE_CUDA "Support CUDA compilation" OFF)
option(PARAVIEW_USE_VTKM "Enable VTK-m accelerated algorithms" ON)

vtk_deprecated_setting(python_default PARAVIEW_USE_PYTHON PARAVIEW_ENABLE_PYTHON OFF)
option(PARAVIEW_USE_PYTHON "Enable/Disable Python scripting support" ${python_default})

vtk_deprecated_setting(qt_gui_default PARAVIEW_USE_QT PARAVIEW_BUILD_QT_GUI "ON")
option(PARAVIEW_USE_QT "Enable Qt-support needed for graphical UI" ${qt_gui_default})

# Add an option to enable using Qt Webkit for widgets, as needed.
# Default is OFF. We don't want to depend on WebKit unless absolutely needed.
# FIXME: Move this to the module which cares.
cmake_dependent_option(PARAVIEW_USE_QTWEBENGINE
  "Use Qt WebKit components as needed." OFF
  "PARAVIEW_USE_QT" OFF)
mark_as_advanced(PARAVIEW_USE_QTWEBENGINE)

# Add an option to enable using Qt Help, as needed.
# Default is ON to enable integrated help/documentation.
cmake_dependent_option(PARAVIEW_USE_QTHELP
  "Use Qt Help infrastructure as needed." ON
  "PARAVIEW_USE_QT" OFF)
mark_as_advanced(PARAVIEW_USE_QTHELP)

#========================================================================
# FEATURE OPTIONS:
# Options that toggle features. These should begin with `PARAVIEW_ENABLE_`.
#========================================================================

vtk_deprecated_setting(raytracing_default PARAVIEW_ENABLE_RAYTRACING PARAVIEW_USE_RAYTRACING "OFF")
option(PARAVIEW_ENABLE_RAYTRACING "Build ParaView with OSPray and/or OptiX ray-tracing support")

# It's arguable if logging is a capability rather than a feature; however since
# it's simply results in enabling/disabling a module I am leaving it as a
# feature.
option(PARAVIEW_ENABLE_LOGGING "Enable logging support" ON)
mark_as_advanced(PARAVIEW_ENABLE_LOGGING)

set(paraview_web_default ON)
if (PARAVIEW_USE_PYTHON AND WIN32)
  include("${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake/FindPythonModules.cmake")
  find_python_module(win32api have_pywin32)
  set(paraview_web_default "${have_pywin32}")
endif ()

if (PARAVIEW_BUILD_ESSENTIALS_ONLY)
  set(paraview_web_default OFF)
endif()
cmake_dependent_option(PARAVIEW_ENABLE_WEB "Enable/Disable web support" "${paraview_web_default}"
  "PARAVIEW_USE_PYTHON" OFF)
mark_as_advanced(PARAVIEW_ENABLE_WEB)

# NvPipe requires an NVIDIA GPU.
option(PARAVIEW_ENABLE_NVPIPE "Build ParaView with NvPipe remoting. Requires CUDA and an NVIDIA GPU" OFF)
mark_as_advanced(PARAVIEW_ENABLE_NVPIPE)

option(PARAVIEW_ENABLE_GDAL "Enable GDAL support." OFF)
mark_as_advanced(PARAVIEW_ENABLE_GDAL)

option(PARAVIEW_ENABLE_LAS "Enable LAS support." OFF)
mark_as_advanced(PARAVIEW_ENABLE_LAS)

option(PARAVIEW_ENABLE_OPENTURNS "Enable OpenTURNS support." OFF)
mark_as_advanced(PARAVIEW_ENABLE_OPENTURNS)

option(PARAVIEW_ENABLE_PDAL "Enable PDAL support." OFF)
mark_as_advanced(PARAVIEW_ENABLE_PDAL)

option(PARAVIEW_ENABLE_MOTIONFX "Enable MotionFX support." OFF)
mark_as_advanced(PARAVIEW_ENABLE_MOTIONFX)

option(PARAVIEW_ENABLE_MOMENTINVARIANTS "Enable MomentInvariants filters" OFF)
mark_as_advanced(PARAVIEW_ENABLE_MOMENTINVARIANTS)

option(PARAVIEW_ENABLE_VISITBRIDGE "Enable VisIt readers." OFF)
mark_as_advanced(PARAVIEW_ENABLE_VISITBRIDGE)

option(PARAVIEW_ENABLE_XDMF2 "Enable Xdmf2 support." ON)
mark_as_advanced(PARAVIEW_ENABLE_XDMF2)

option(PARAVIEW_ENABLE_XDMF3 "Enable Xdmf3 support." OFF)
mark_as_advanced(PARAVIEW_ENABLE_XDMF3)

option(PARAVIEW_ENABLE_ADIOS2 "Enable ADIOS 2.x support." OFF)
mark_as_advanced(PARAVIEW_ENABLE_ADIOS2)

cmake_dependent_option(PARAVIEW_ENABLE_FFMPEG "Enable FFMPEG Support." OFF
  "UNIX" OFF)

# If building on Unix with MPI enabled, we will present another option to
# enable building of CosmoTools VTK extensions. This option is by default
# OFF and set to OFF if ParaView is not built with MPI.
cmake_dependent_option(PARAVIEW_ENABLE_COSMOTOOLS
  "Build ParaView with CosmoTools VTK Extensions" OFF
  "UNIX;PARAVIEW_USE_MPI" OFF)
mark_as_advanced(PARAVIEW_ENABLE_COSMOTOOLS)

#========================================================================
# NON-ESSENTIAL FEATURES:
# Features without external dependencies.
# These features are generally turned ON by default unless
# PARAVIEW_BUILD_ESSENTIALS_ONLY is true. In which case, these are exposed as
# user settable options.
#========================================================================

macro(paraview_essentials_only_option name doc default)
  cmake_dependent_option(${name} "${doc}" "${default}"
    "PARAVIEW_BUILD_ESSENTIALS_ONLY" ON)
endmacro()

macro(paraview_essentials_only_dependent_option option doc default depends force)
  if (PARAVIEW_BUILD_ESSENTIALS_ONLY)
    cmake_dependent_option(${option} "${doc}" "${default}" "${depends}" "${force}")
  else()
    # When not building essentials, we are not providing the option at all.
    set(${option}_AVAILABLE 1)
    foreach(d ${depends})
      string(REGEX REPLACE " +" ";" CMAKE_DEPENDENT_OPTION_DEP "${d}")
      if(${CMAKE_DEPENDENT_OPTION_DEP})
      else()
        set(${option}_AVAILABLE 0)
      endif()
    endforeach()
    set(${option} "${${option}}" CACHE INTERNAL "${doc}")
    if (${option}_AVAILABLE)
      set(${option} ON)
    else()
      set(${option} ${force})
    endif()
  endif()
endmacro()

set (essential_default ON)
set (nonessential_default ON)
if (PARAVIEW_BUILD_ESSENTIALS_ONLY)
  set (nonessential_default OFF)
endif()

paraview_essentials_only_option(PARAVIEW_ENABLE_RENDERING
  "Enable ParaView rendering support." ON)

paraview_essentials_only_option(PARAVIEW_ENABLE_OPTIONAL_IO
  "Enable non-essential IO support." OFF)

paraview_essentials_only_dependent_option(PARAVIEW_ENABLE_CINEMA_IMPORTER
  "Enable Cinema database importer." OFF
  "PARAVIEW_USE_PYTHON;PARAVIEW_ENABLE_RENDERING" OFF)

paraview_essentials_only_dependent_option(PARAVIEW_ENABLE_CINEMA_EXPORTER
  "Enable Cinema database exporter." OFF
  "PARAVIEW_USE_PYTHON" OFF)


#========================================================================
# MISCELLANEOUS OPTIONS:
# Options that are hard to classify. Keep this list minimal.
# These should be advanced by default.
#========================================================================
option(PARAVIEW_INSTALL_DEVELOPMENT_FILES "Install development files to the install tree" ON)
mark_as_advanced(PARAVIEW_INSTALL_DEVELOPMENT_FILES)

option(PARAVIEW_RELOCATABLE_INSTALL "Do not embed hard-coded paths into the install" ON)
mark_as_advanced(PARAVIEW_RELOCATABLE_INSTALL)


cmake_dependent_option(PARAVIEW_INITIALIZE_MPI_ON_CLIENT
  "Initialize MPI on client-processes by default. Can be overridden using command line arguments" ON
  "PARAVIEW_USE_MPI" OFF)
mark_as_advanced(PARAVIEW_INITIALIZE_MPI_ON_CLIENT)

#========================================================================
# OBSOLETE OPTIONS: mark obsolete settings
#========================================================================
vtk_obsolete_setting(PARAVIEW_ENABLE_QT_SUPPORT)
vtk_obsolete_setting(PARAVIEW_ENABLE_COMMANDLINE_TOOLS)
vtk_obsolete_setting(PARAVIEW_FREEZE_PYTHON)
vtk_obsolete_setting(PARAVIEW_USE_MPI_SSEND)
vtk_obsolete_setting(PARAVIEW_USE_ICE_T)
vtk_obsolete_setting(PARAVIEW_ENABLE_CATALYST)

#========================================================================================
# Build up list of required and rejected modules
#========================================================================================
set(paraview_requested_modules)
set(paraview_rejected_modules)

#[==[
Conditionally require/reject optional modules

Use this macro to conditionally require (or reject) modules.

~~~
paraview_require_module(
  MODULES             <module>...
  [CONDITION          <condition>]
  [CONDITION_DEFAULT  <condition>])
~~~

The arguments are as follows:

  * `MODULES`: (Required) The list of modules.
  * `CONDITION`: (Defaults to `TRUE`) The condition under which the modules
    specified are added to the requested list or rejected list.
  * `CONDITION_DEFAULT`: (Defaults to `TRUE`) This is secondary condition which
    must be true for the module(s) to be added to the required list. If this is
    false, but `CONDITION` is true, then the module is added neither to the
    requested list nor the rejected list i.e. the user can manually enable them,
    if needed.
#]==]
macro (paraview_require_module)
  cmake_parse_arguments(pem
    ""
    ""
    "CONDITION;CONDITION_DEFAULT;MODULES"
    ${ARGN})

  if (pem_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for `paraview_require_module`: "
      "${pem_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED pem_CONDITION)
    set(pem_CONDITION TRUE)
  endif ()

  if (NOT DEFINED pem_CONDITION_DEFAULT)
    set(pem_CONDITION_DEFAULT TRUE)
  endif ()

  if (${pem_CONDITION})
    # message("${pem_CONDITION} == TRUE")
    # if CONDITION_DEFAULT is specified, the module is added to requested
    # modules only if the condition provided is true, otherwise it's simply
    # skipped. Note, it's not added to rejected since we don't want to reject
    # it.
    if (${pem_CONDITION_DEFAULT})
      list(APPEND paraview_requested_modules ${pem_MODULES})
    endif()
  else ()
    # message("${pem_CONDITION} == FALSE")
    list(APPEND paraview_rejected_modules ${pem_MODULES})
  endif()
  unset(pem_CONDITION)
  unset(pem_CONDITION_DEFAULT)
  unset(pem_MODULES)
  unset(pem_UNPARSED_ARGUMENTS)
endmacro()

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_LOGGING
  MODULES VTK::loguru)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_CINEMA_IMPORTER
  MODULES   ParaView::CinemaReader
            VTK::IOAsynchronous)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_CINEMA_EXPORTER
  MODULES   ParaView::CinemaPython
            VTK::IOAsynchronous)

paraview_require_module(
  CONDITION PARAVIEW_USE_PYTHON
  MODULES   ParaView::PythonAlgorithm
            ParaView::PythonInitializer
            VTK::PythonInterpreter)

paraview_require_module(
  CONDITION         PARAVIEW_USE_PYTHON
  CONDITION_DEFAULT PARAVIEW_ENABLE_RENDERING
  MODULES           ParaView::ClientServerCorePythonRendering
                    VTK::RenderingMatplotlib)

paraview_require_module(
  CONDITION PARAVIEW_USE_QT
  MODULES   ParaView::pqApplicationComponents
            ParaView::pqComponents
            ParaView::pqCore
            ParaView::pqWidgets
            ParaView::qttesting
            VTK::GUISupportQt)

paraview_require_module(
  CONDITION PARAVIEW_USE_QT AND PARAVIEW_USE_PYTHON
  MODULES   ParaView::pqPython)

paraview_require_module(
  CONDITION PARAVIEW_USE_MPI
  MODULES   ParaView::icet
            VTK::FiltersParallelGeometry
            VTK::FiltersParallelMPI
            VTK::mpi
            VTK::ParallelMPI)

paraview_require_module(
  CONDITION         PARAVIEW_USE_MPI
  CONDITION_DEFAULT PARAVIEW_ENABLE_OPTIONAL_IO
  MODULES           VTK::IOMPIImage
                    VTK::IOParallelLSDyna
                    VTK::IOParallelNetCDF)

paraview_require_module(
  CONDITION PARAVIEW_USE_MPI AND PARAVIEW_USE_PYTHON
  MODULES   VTK::ParallelMPI4Py)

paraview_require_module(
  CONDITION PARAVIEW_USE_VTKM
  MODULES   VTK::AcceleratorsVTKm)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_RAYTRACING
  MODULES   VTK::RenderingRayTracing)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_NVPIPE
  MODULES   ParaView::nvpipe)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_GDAL
  MODULES   VTK::IOGDAL)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_LAS
  MODULES   VTK::IOLAS)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_OPENTURNS
  MODULES   VTK::FiltersOpenTURNS)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_PDAL
  MODULES   VTK::IOPDAL)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_MOTIONFX
  MODULES   VTK::IOMotionFX)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_MOMENTINVARIANTS
  MODULES   VTK::MomentInvariants)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_MOMENTINVARIANTS AND PARAVIEW_USE_MPI
  MODULES   VTK::ParallelMomentInvariants)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_VISITBRIDGE
  MODULES   ParaView::IOVisItBridge
            ParaView::VisItLib)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_XDMF2
  MODULES   VTK::IOXdmf2)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_XDMF3
  MODULES   VTK::IOXdmf3)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_ADIOS2
  MODULES   VTK::IOADIOS2)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_FFMPEG
  MODULES   VTK::IOFFMPEG)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_COSMOTOOLS
  MODULES   ParaView::VTKExtensionsCosmoTools)

paraview_require_module(
  CONDITION PARAVIEW_USE_PYTHON
  MODULES   ParaView::PythonCatalyst)

paraview_require_module(
  CONDITION PARAVIEW_BUILD_TESTING
  MODULES   ParaView::CatalystTestDriver
            ParaView::smTestDriver)

paraview_require_module(
  CONDITION_DEFAULT NOT PARAVIEW_BUILD_ESSENTIALS_ONLY
  MODULES           ParaView::ServerManagerDefault
                    ParaView::VTKExtensionsPoints
                    ParaView::Animation
                    VTK::FiltersFlowPaths
                    VTK::FiltersParallelDIY2
                    VTK::FiltersParallelVerdict
                    VTK::FiltersFlowPaths)

paraview_require_module(
  CONDITION         PARAVIEW_USE_MPI
  CONDITION_DEFAULT NOT PARAVIEW_BUILD_ESSENTIALS_ONLY
  MODULES           ParaView::VTKExtensionsDefaultParallel
                    VTK::FiltersParallelFlowPaths)

paraview_require_module(
  MODULES   ParaView::Catalyst
            ParaView::ProcessXML
            ParaView::ServerManagerCore
            ParaView::ServerManagerApplication
            ParaView::WrapClientServer
            VTK::FiltersTexture)

paraview_require_module(
  CONDITION_DEFAULT PARAVIEW_ENABLE_OPTIONAL_IO
  MODULES           ParaView::VTKExtensionsCGNSReader
                    ParaView::VTKExtensionsCGNSWriter
                    VTK::IOAMR
                    VTK::IOCityGML
                    VTK::IOH5part
                    VTK::IOParallelLSDyna
                    VTK::IOSegY
                    VTK::IOTecplotTable
                    VTK::IOTRUCHAS
                    VTK::IOVeraOut
                    VTK::IOVPIC)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_WEB
  MODULES   ParaView::PVWebCore
            ParaView::PVWebExporter
            ParaView::PVWebPython
            VTK::WebCore
            VTK::WebPython)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_WEB AND (PARAVIEW_PYTHON_VERSION STREQUAL "2")
  MODULES   ParaView::PVWebPython2)

paraview_require_module(
  CONDITION_DEFAULT PARAVIEW_ENABLE_RENDERING
  MODULES           ParaView::ServerManagerRendering)


if (paraview_requested_modules)
  list(REMOVE_DUPLICATES paraview_requested_modules)
endif ()

if (paraview_rejected_modules)
  list(REMOVE_DUPLICATES paraview_rejected_modules)
endif()

# message(STATUS "REQUESTED: ${paraview_requested_modules}")
# message(STATUS "REJECTED: ${paraview_rejected_modules}")
