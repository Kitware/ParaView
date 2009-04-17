# +---------------------------------------------------------------------------+
# |                                                                           |
# |                            VisIt Configuration                            |
# |                                                                           |
# +---------------------------------------------------------------------------+
#
# Purpose:
#
# Generates the configuration variables:
#   VISIT_BASE                  -- /path/to/VisIt/source/tree
#   VISIT_LIB_PATH              -- /path/to/VisIt/libs
#   VISIT_BIN_PATH              -- /path/to/VisIt/bins (Windows only)
#   VISIT_INCLUDE_PATH          -- /path/to/VisIt/includes (list)
#   VISIT_LIBS                  -- libs (list)
#   VISIT_WITH_MPI              -- use lib*_par.so or lib*_ser.so
#   VISIT_PLUGIN_BIN            -- /path/to/VisIt/plugins
#   VISIT_THIRD_PARTY_BIN       -- /path/to/prebuilt/dependencies (Windows only)
#   VISIT_THIRD_PARTY_LIB       -- /path/to/prebuilt/dependencies (Windows only)
#

# Point to the location of the VisIt sources and VisIt library build.
if (UNIX OR CYGWIN)
# Unix/Linux/Cygwin
set(VISIT_BASE /home/burlen/ext2/v3/visit1.10.0 CACHE FIELPATH
    "Path to VisIt1.10.0 source.")
else (UNIX OR CYGWIN)
# Windows
set(VISIT_BASE C:/VisItDev1.10.0.X CACHE FIELPATH
    "Path to VisIt1.10.0 source.")
endif (UNIX OR CYGWIN)

if (NOT EXISTS ${VISIT_BASE})
  MESSAGE( FATAL_ERROR 
  "Set VISIT_BASE to the path to your local VisIt 1.10.0 sources." )
endif (NOT EXISTS ${VISIT_BASE})

# Specify the subset of the VisIt lib's which are needed.
set(VISIT_WITH_MPI OFF CACHE BOOL "Set ON if you built VisIt with MPI")

set(VISIT_INCLUDE_PATH
  ${VISIT_BASE}/windowsbuild/include/VisIt
  ${VISIT_BASE}/src/databases/ANALYZE
  ${VISIT_BASE}/src/avt/Database/Database
  ${VISIT_BASE}/src/avt/Database/Formats
  ${VISIT_BASE}/src/avt/Database/Ghost
  ${VISIT_BASE}/src/avt/DBAtts/MetaData
  ${VISIT_BASE}/src/avt/DBAtts/SIL
  ${VISIT_BASE}/src/avt/DDF
  ${VISIT_BASE}/src/avt/Expressions/Abstract
  ${VISIT_BASE}/src/avt/Expressions/CMFE
  ${VISIT_BASE}/src/avt/Expressions/Conditional
  ${VISIT_BASE}/src/avt/Expressions/Derivations
  ${VISIT_BASE}/src/avt/Expressions/General
  ${VISIT_BASE}/src/avt/Expressions/ImageProcessing
  ${VISIT_BASE}/src/avt/Expressions/Management
  ${VISIT_BASE}/src/avt/Expressions/Math
  ${VISIT_BASE}/src/avt/Expressions/MeshQuality
  ${VISIT_BASE}/src/avt/FileWriter
  ${VISIT_BASE}/src/avt/Filters
  ${VISIT_BASE}/src/avt/IVP
  ${VISIT_BASE}/src/avt/Math
  ${VISIT_BASE}/src/avt/MIR/Base
  ${VISIT_BASE}/src/avt/MIR/Tet
  ${VISIT_BASE}/src/avt/MIR/Zoo
  ${VISIT_BASE}/src/avt/Pipeline/AbstractFilters
  ${VISIT_BASE}/src/avt/Pipeline/Data
  ${VISIT_BASE}/src/avt/Pipeline/Pipeline
  ${VISIT_BASE}/src/avt/Pipeline/Sinks
  ${VISIT_BASE}/src/avt/Pipeline/Sources
  ${VISIT_BASE}/src/avt/Plotter
  ${VISIT_BASE}/src/avt/Preprocessor
  ${VISIT_BASE}/src/avt/QtVisWindow
  ${VISIT_BASE}/src/avt/Queries/Abstract
  ${VISIT_BASE}/src/avt/Queries/Misc
  ${VISIT_BASE}/src/avt/Queries/Pick
  ${VISIT_BASE}/src/avt/Queries/Queries
  ${VISIT_BASE}/src/avt/Shapelets
  ${VISIT_BASE}/src/avt/View
  ${VISIT_BASE}/src/avt/VisWindow/Colleagues
  ${VISIT_BASE}/src/avt/VisWindow/Interactors
  ${VISIT_BASE}/src/avt/VisWindow/Proxies
  ${VISIT_BASE}/src/avt/VisWindow/Tools
  ${VISIT_BASE}/src/avt/VisWindow/VisWindow
  ${VISIT_BASE}/src/common
  ${VISIT_BASE}/src/common/comm
  ${VISIT_BASE}/src/common/Exceptions/Database
  ${VISIT_BASE}/src/common/Exceptions/Pipeline
  ${VISIT_BASE}/src/common/Exceptions/Plotter
  ${VISIT_BASE}/src/common/Exceptions/VisWindow
  ${VISIT_BASE}/src/common/expr
  ${VISIT_BASE}/src/common/expr
  ${VISIT_BASE}/src/common/icons
  ${VISIT_BASE}/src/common/misc
  ${VISIT_BASE}/src/common/parser
  ${VISIT_BASE}/src/common/plugin
  ${VISIT_BASE}/src/common/proxybase
  ${VISIT_BASE}/src/common/siloobj
  ${VISIT_BASE}/src/common/siloobj_vtk_db
  ${VISIT_BASE}/src/common/state
  ${VISIT_BASE}/src/common/utility
  ${VISIT_BASE}/src/engine/main
  ${VISIT_BASE}/src/engine/parstate
  ${VISIT_BASE}/src/engine/proxy
  ${VISIT_BASE}/src/engine/rpc
  ${VISIT_BASE}/src/gui
  ${VISIT_BASE}/src/launcher/main
  ${VISIT_BASE}/src/launcher/proxy
  ${VISIT_BASE}/src/launcher/rpc
  ${VISIT_BASE}/src/mdserver/main
  ${VISIT_BASE}/src/mdserver/proxy
  ${VISIT_BASE}/src/mdserver/rpc
  ${VISIT_BASE}/src/sim/examples
  ${VISIT_BASE}/src/sim/lib
  ${VISIT_BASE}/src/third_party_builtin/bow
  ${VISIT_BASE}/src/tools/DataManualExamples
  ${VISIT_BASE}/src/tools/annotations
  ${VISIT_BASE}/src/tools/avt_do_add
  ${VISIT_BASE}/src/tools/clidriver
  ${VISIT_BASE}/src/tools/clipeditor
  ${VISIT_BASE}/src/tools/doxygenate
  ${VISIT_BASE}/src/tools/escan
  ${VISIT_BASE}/src/tools/imagetools
  ${VISIT_BASE}/src/tools/mpeg2encode
  ${VISIT_BASE}/src/tools/mpeg_encode
  ${VISIT_BASE}/src/tools/prep
  ${VISIT_BASE}/src/tools/qtssh/remotecommand
  ${VISIT_BASE}/src/tools/silex
  ${VISIT_BASE}/src/tools/windowmaker
  ${VISIT_BASE}/src/tools/writer
  ${VISIT_BASE}/src/tools/xml
  ${VISIT_BASE}/src/tools/xmledit
  ${VISIT_BASE}/src/viewer/main
  ${VISIT_BASE}/src/viewer/proxy
  ${VISIT_BASE}/src/viewer/rpc
  ${VISIT_BASE}/src/visit_vtk/full
  ${VISIT_BASE}/src/visit_vtk/lightweight
  ${VISIT_BASE}/src/visit_vtk/parallel
  ${VISIT_BASE}/src/visitpy/common
  ${VISIT_BASE}/src/visitpy/visitpy
  ${VISIT_BASE}/src/vtkqt
  ${VISIT_BASE}/src/winutil)

# Configure the VisIt Libraries. A couple of choices to make
# based on the platform, and build options. The Windows build
# configuration differs substantially from the Linux/Mac build
# configuration, therefor our configuration is split by platform.
if (UNIX OR CYGWIN)
  # +------------------+
  # | Unix/Linux/Cygwin|
  # +------------------+
  set(VISIT_PLUGIN_BIN   "${VISIT_BASE}/src/plugins")
  set(VISIT_LIB_PATH     "${VISIT_BASE}/src/lib")
  #set(VISIT_INCLUDE_PATH "${VISIT_BASE}/src/include/visit)
  set(VISIT_THIRD_PARTY_BIN)
  set(VISIT_LIBS
    lightweight_visit_vtk
    visit_vtk
    avtexceptions
    avtview
    bow
    comm
    dbatts
    expr
    misc
    parser
    plugin
    proxybase
    state
    utility)
    if (VISIT_WITH_MPI)
    # Use VisIt's parallel libraries.
    message(STATUS "Using VisIt parallel Linux libraries.")
    set(VISIT_LIBS
      ${VISIT_LIBS}
      plotter_par
      avtddf_par
      avtfilters_par
      avtivp_par
      avtmath_par
      avtshapelets_par
      avtwriter_par
      database_par
      expressions_par
      mir_par
      pipeline_par
      query_par)
    else (VISIT_WITH_MPI)
    # USe VisIt's serial libraries.
    message(STATUS "Using VisIt serial Linux libraries.")
    set(VISIT_LIBS
      ${VISIT_LIBS}
      plotter_ser
      avtddf_ser
      avtfilters_ser
      avtivp_ser
      avtmath_ser
      avtshapelets_ser
      avtwriter_ser
      database_ser
      expressions_ser
      mir_ser
      pipeline_ser
      query_ser)
    endif (VISIT_WITH_MPI)
else (UNIX OR CYGWIN)
  # +------------------+
  # |     Windows      |
  # +------------------+
  set(VISIT_BUILD_COMPILER "MSVC8.Net" CACHE STRING "Set VisIt's compiler type. (eg. MSVC8.Net)")
  #
  set(VISIT_LIB_PATH "${VISIT_BASE}/windowsbuild/lib/${VISIT_BUILD_COMPILER}")
  set(VISIT_BIN_PATH "${VISIT_BASE}/windowsbuild/bin/${VISIT_BUILD_COMPILER}")
  set(VISIT_PLUGIN_BIN "${VISIT_BIN_PATH}/${PLUGIN_BUILD_TYPE}")
  set(VISIT_THIRD_PARTY_BIN "${VISIT_BIN_PATH}/ThirdParty")
  set(VISIT_THIRD_PARTY_LIB "${VISIT_LIB_PATH}/ThirdParty")
  #
  set(VISIT_LIBS
    visit_vtk_light
    visit_vtk
    avtexceptions  
    avtview
    comm
    dbatts
    visitexpr
    visitparser
    expression
    misc
    plugin
    proxybase
    state
    utility)
    if (VISIT_WITH_MPI)
    # Use VisIt's parallel libraries.
    message(STATUS "Using VisIt parallel Windows libraries.")
    set(VISIT_LIBS
      ${VISIT_LIBS}
      plotter_par
      avtddf_par
      avtfilters_par
      avtivp_par
      avtmath_par
      avtshapelets_par
      avtwriter_par
      database_par
      expressions_par
      mir_par
      pipeline_par
      query_par)
    else (VISIT_WITH_MPI)
    # USe VisIt's serial libraries.
    message(STATUS "Using VisIt serial Windows libraries.")
    set(VISIT_LIBS
      ${VISIT_LIBS}
      plotter
      ddf
      avtfilters
      avtivp
      avtmath_ser
      shapelets
      database_ser
      mir
      pipeline_ser
      queries)
    endif (VISIT_WITH_MPI)
endif (UNIX OR CYGWIN)
