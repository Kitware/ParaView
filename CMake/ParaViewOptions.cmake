#========================================================================
# BUILD OPTIONS:
# Options that affect the ParaView build, in general.
# These should begin with `PARAVIEW_BUILD_`.
#========================================================================

vtk_deprecated_setting(shared_default PARAVIEW_BUILD_SHARED_LIBS BUILD_SHARED_LIBS "ON")
option(PARAVIEW_BUILD_SHARED_LIBS "Build ParaView with shared libraries" "${shared_default}")

vtk_deprecated_setting(legacy_remove_default PARAVIEW_BUILD_LEGACY_REMOVE VTK_LEGACY_REMOVE "OFF")
option(PARAVIEW_BUILD_LEGACY_REMOVE "Remove all legacy code completely" "${legacy_remove_default}")
mark_as_advanced(PARAVIEW_BUILD_LEGACY_REMOVE)

vtk_deprecated_setting(legacy_silent_default PARAVIEW_BUILD_LEGACY_SILENT VTK_LEGACY_SILENT "OFF")
option(PARAVIEW_BUILD_LEGACY_SILENT "Silence all legacy code messages" "${legacy_silent_default}")
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
option(PARAVIEW_BUILD_DEVELOPER_DOCUMENTATION "Generate ParaView C++/Python docs" "${doc_default}")

set(PARAVIEW_BUILD_ESSENTIALS "CANONICAL"
  CACHE STRING "Enable ParaView components essential for requested capabilities.")
set_property(CACHE PARAVIEW_BUILD_ESSENTIALS
  PROPERTY
    STRINGS "CORE;RENDERING;CANONICAL")

if (PARAVIEW_BUILD_ESSENTIALS STREQUAL "CORE")
  set(VTK_GROUP_ENABLE_PARAVIEW_CORE "YES" CACHE INTERNAL "")
  set(VTK_GROUP_ENABLE_PARAVIEW_RENDERING "NO" CACHE INTERNAL "")
  set(VTK_GROUP_ENABLE_PARAVIEW_CANONICAL "NO" CACHE INTERNAL "")
  set(PARAVIEW_BUILD_CORE ON)
  set(PARAVIEW_BUILD_RENDERING OFF)
  set(PARAVIEW_BUILD_CANONICAL OFF)
elseif (PARAVIEW_BUILD_ESSENTIALS STREQUAL "RENDERING")
  set(VTK_GROUP_ENABLE_PARAVIEW_CORE "YES" CACHE INTERNAL "")
  set(VTK_GROUP_ENABLE_PARAVIEW_RENDERING "YES" CACHE INTERNAL "")
  set(VTK_GROUP_ENABLE_PARAVIEW_CANONICAL "NO" CACHE INTERNAL "")
  set(PARAVIEW_BUILD_CORE ON)
  set(PARAVIEW_BUILD_RENDERING ON)
  set(PARAVIEW_BUILD_CANONICAL OFF)
elseif (PARAVIEW_BUILD_ESSENTIALS STREQUAL "CANONICAL")
  set(VTK_GROUP_ENABLE_PARAVIEW_CORE "YES" CACHE INTERNAL "")
  set(VTK_GROUP_ENABLE_PARAVIEW_RENDERING "YES" CACHE INTERNAL "")
  set(VTK_GROUP_ENABLE_PARAVIEW_CANONICAL "YES" CACHE INTERNAL "")
  set(PARAVIEW_BUILD_CORE ON)
  set(PARAVIEW_BUILD_RENDERING ON)
  set(PARAVIEW_BUILD_CANONICAL ON)
endif()


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
option(PARAVIEW_USE_PYTHON "Enable/Disable Python scripting support" "${python_default}")

# Currently, we're making `PARAVIEW_USE_QT` available only when doing CANONICAL
# builds. This is technically not necessary so we can support that use-case if
# needed in future but will require some work to make sure the Qt components
# work correctly with missing proxies.
vtk_deprecated_setting(qt_gui_default PARAVIEW_USE_QT PARAVIEW_BUILD_QT_GUI "ON")
cmake_dependent_option(PARAVIEW_USE_QT
  "Enable Qt-support needed for graphical UI" "${qt_gui_default}"
  "PARAVIEW_BUILD_CANONICAL" OFF)

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

set(paraview_web_default ON)
if (PARAVIEW_USE_PYTHON AND WIN32)
  include("${CMAKE_CURRENT_SOURCE_DIR}/VTK/CMake/FindPythonModules.cmake")
  find_python_module(win32api have_pywin32)
  set(paraview_web_default "${have_pywin32}")
endif ()

if (NOT PARAVIEW_BUILD_ESSENTIALS STREQUAL "CANONICAL")
  set(paraview_web_default OFF)
endif()
cmake_dependent_option(PARAVIEW_ENABLE_WEB "Enable/Disable web support" "${paraview_web_default}"
  "PARAVIEW_USE_PYTHON" OFF)

# NvPipe requires an NVIDIA GPU.
option(PARAVIEW_ENABLE_NVPIPE "Build ParaView with NvPipe remoting. Requires CUDA and an NVIDIA GPU" OFF)

option(PARAVIEW_ENABLE_GDAL "Enable GDAL support." OFF)

option(PARAVIEW_ENABLE_LAS "Enable LAS support." OFF)

option(PARAVIEW_ENABLE_OPENTURNS "Enable OpenTURNS support." OFF)

option(PARAVIEW_ENABLE_PDAL "Enable PDAL support." OFF)

option(PARAVIEW_ENABLE_MOTIONFX "Enable MotionFX support." OFF)

option(PARAVIEW_ENABLE_MOMENTINVARIANTS "Enable MomentInvariants filters" OFF)

option(PARAVIEW_ENABLE_VISITBRIDGE "Enable VisIt readers." OFF)

option(PARAVIEW_ENABLE_XDMF2 "Enable Xdmf2 support." ON)

option(PARAVIEW_ENABLE_XDMF3 "Enable Xdmf3 support." OFF)

option(PARAVIEW_ENABLE_ADIOS2 "Enable ADIOS 2.x support." OFF)

cmake_dependent_option(PARAVIEW_ENABLE_FFMPEG "Enable FFMPEG Support." OFF
  "UNIX" OFF)

# If building on Unix with MPI enabled, we will present another option to
# enable building of CosmoTools VTK extensions. This option is by default
# OFF and set to OFF if ParaView is not built with MPI.
cmake_dependent_option(PARAVIEW_ENABLE_COSMOTOOLS
  "Build ParaView with CosmoTools VTK Extensions" OFF
  "UNIX;PARAVIEW_USE_MPI" OFF)

# PARAVIEW_ENABLE_EXPORTERS option is shown only when PARAVIEW_BUILD_ESSENTIALS is set to RENDERING.
# If PARAVIEW_BUILD_ESSENTIALS is set to CORE, the ParaView::RemotingExport module's CONDITION
# ensures that the module is not built nor is the option available.
# If PARAVIEW_BUILD_ESSENTIALS is set to CANONICAL, the option is not shown but is assumed ON and
# ParaView::RemotingExport module's CONDITION succeeds, thus enabling the module.
cmake_dependent_option(PARAVIEW_ENABLE_EXPORTERS
  "Enable view exporters." OFF
  "NOT PARAVIEW_BUILD_CANONICAL;PARAVIEW_BUILD_RENDERING" ON)

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
vtk_obsolete_setting(PARAVIEW_ENABLE_LOGGING)
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
  [EXCLUSIVE]
~~~

The arguments are as follows:

  * `MODULES`: (Required) The list of modules.
  * `CONDITION`: (Defaults to `TRUE`) The condition under which the modules
    specified are added to the requested list.
  * `EXCLUSIVE`: When sepcified, if `CONDITION` is false, the module will be
    added to the rejected modules list.
#]==]
macro (paraview_require_module)
  cmake_parse_arguments(pem
    "EXCLUSIVE"
    ""
    "CONDITION;MODULES"
    ${ARGN})

  if (pem_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for `paraview_require_module`: "
      "${pem_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED pem_CONDITION)
    set(pem_CONDITION TRUE)
  endif ()

  if (${pem_CONDITION})
    # message("${pem_CONDITION} == TRUE")
    list(APPEND paraview_requested_modules ${pem_MODULES})
  elseif (pem_EXCLUSIVE)
    # message("${pem_CONDITION} == FALSE")
    list(APPEND paraview_rejected_modules ${pem_MODULES})
  endif()
  unset(pem_EXCLUSIVE)
  unset(pem_CONDITION)
  unset(pem_MODULES)
  unset(pem_UNPARSED_ARGUMENTS)
endmacro()

# ensures that VTK::mpi module is rejected when MPI is not enabled.
paraview_require_module(
  CONDITION PARAVIEW_USE_MPI
  MODULES   VTK::mpi
  EXCLUSIVE)

# ensures VTK::Python module is rejected when Python is not enabled.
paraview_require_module(
  CONDITION PARAVIEW_USE_PYTHON
  MODULES   VTK::Python
            VTK::PythonInterpreter
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_USE_PYTHON AND PARAVIEW_BUILD_RENDERING
  MODULES   VTK::RenderingMatplotlib)

paraview_require_module(
  CONDITION PARAVIEW_USE_VTKM
  MODULES   VTK::AcceleratorsVTKm
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_RAYTRACING AND PARAVIEW_BUILD_RENDERING
  MODULES   VTK::RenderingRayTracing
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_NVPIPE
  MODULES   ParaView::nvpipe
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_GDAL
  MODULES   VTK::IOGDAL
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_LAS
  MODULES   VTK::IOLAS
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_OPENTURNS
  MODULES   VTK::FiltersOpenTURNS
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_PDAL
  MODULES   VTK::IOPDAL
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_MOTIONFX
  MODULES   VTK::IOMotionFX
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_MOMENTINVARIANTS
  MODULES   VTK::MomentInvariants
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_MOMENTINVARIANTS AND PARAVIEW_USE_MPI
  MODULES   VTK::ParallelMomentInvariants
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_VISITBRIDGE
  MODULES   ParaView::IOVisItBridge
            ParaView::VisItLib
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_XDMF2
  MODULES   VTK::IOXdmf2
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_XDMF3
  MODULES   VTK::IOXdmf3
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_ADIOS2
  MODULES   VTK::IOADIOS2
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_FFMPEG
  MODULES   VTK::IOFFMPEG
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_ENABLE_WEB AND PARAVIEW_USE_PYTHON
  MODULES   VTK::WebCore
            VTK::WebPython
  EXCLUSIVE)

paraview_require_module(
  CONDITION PARAVIEW_BUILD_CANONICAL
  MODULES ParaView::VTKExtensionsFiltersGeneral
          VTK::DomainsChemistry
          VTK::FiltersAMR
          VTK::FiltersCore
          VTK::FiltersExtraction
          VTK::FiltersFlowPaths
          VTK::FiltersGeneral
          VTK::FiltersGeneric
          VTK::FiltersGeometry
          VTK::FiltersHybrid
          VTK::FiltersHyperTree
          VTK::FiltersModeling
          VTK::FiltersOpenTURNS
          VTK::FiltersParallel
          VTK::FiltersParallelDIY2
          VTK::FiltersParallelVerdict
          VTK::FiltersSources
          VTK::FiltersStatistics
          VTK::FiltersTexture
          VTK::FiltersVerdict
          VTK::ImagingCore
          VTK::ImagingFourier
          VTK::ImagingGeneral
          VTK::ImagingHybrid
          VTK::ImagingSources
          VTK::IOAMR
          VTK::IOAsynchronous # needed for cinema
          VTK::IOCityGML
          VTK::IOGeometry
          VTK::IOH5part
          VTK::IOImage
          VTK::IOInfovis
          VTK::IOLegacy
          VTK::IONetCDF
          VTK::IOOggTheora
          VTK::IOParallel
          VTK::IOParallelExodus
          VTK::IOParallelLSDyna
          VTK::IOParallelXML
          VTK::IOPIO
          VTK::IOSegY
          VTK::IOTecplotTable
          VTK::IOTRUCHAS
          VTK::IOVeraOut
          VTK::IOVPIC
          VTK::IOXML)

paraview_require_module(
  CONDITION PARAVIEW_BUILD_RENDERING
  MODULES   VTK::FiltersTexture
            VTK::RenderingFreeType)

paraview_require_module(
  CONDITION PARAVIEW_USE_MPI AND PARAVIEW_USE_PYTHON
  MODULES   VTK::ParallelMPI4Py)

paraview_require_module(
  CONDITION PARAVIEW_USE_MPI AND PARAVIEW_BUILD_CANONICAL
  MODULES   VTK::FiltersParallelFlowPaths
            VTK::FiltersParallelGeometry
            VTK::FiltersParallelMPI
            VTK::IOMPIImage
            VTK::IOParallelNetCDF)

if (paraview_requested_modules)
  list(REMOVE_DUPLICATES paraview_requested_modules)
endif ()

if (paraview_rejected_modules)
  list(REMOVE_DUPLICATES paraview_rejected_modules)
endif()
