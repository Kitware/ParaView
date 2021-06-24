find_path(Graphviz_INCLUDE_DIR
  NAMES
    cgraph.h
  PATH_SUFFIXES
    graphviz
    )
mark_as_advanced(Graphviz_INCLUDE_DIR)

find_library(Graphviz_CDT_LIBRARY
  NAMES
    cdt
    )
mark_as_advanced(Graphviz_CDT_LIBRARY)

find_library(Graphviz_GVC_LIBRARY
  NAMES
    gvc
    )
mark_as_advanced(Graphviz_GVC_LIBRARY)

find_library(Graphviz_CGRAPH_LIBRARY
  NAMES
    cgraph
    )
mark_as_advanced(Graphviz_CGRAPH_LIBRARY)

find_library(Graphviz_PATHPLAN_LIBRARY
  NAMES
    pathplan
    )
mark_as_advanced(Graphviz_PATHPLAN_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Graphviz
  REQUIRED_VARS
    Graphviz_INCLUDE_DIR
    Graphviz_CDT_LIBRARY
    Graphviz_GVC_LIBRARY
    Graphviz_CGRAPH_LIBRARY
    Graphviz_PATHPLAN_LIBRARY
)

if(Graphviz_FOUND)
  set(Graphviz_INCLUDE_DIRS "${Graphviz_INCLUDE_DIR}")
  set(Graphviz_LIBRARIES
    "${Graphviz_CDT_LIBRARY}"
    "${Graphviz_GVC_LIBRARY}"
    "${Graphviz_CGRAPH_LIBRARY}"
    "${Graphviz_PATHPLAN_LIBRARY}")

  if (NOT TARGET Graphviz::cdt)
    add_library(Graphviz::cdt UNKNOWN IMPORTED)
    set_target_properties(Graphviz::cdt PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Graphviz_INCLUDE_DIR}"
      IMPORTED_LOCATION "${Graphviz_CDT_LIBRARY}")
  endif ()

  if (NOT TARGET Graphviz::pathplan)
    add_library(Graphviz::pathplan UNKNOWN IMPORTED)
    set_target_properties(Graphviz::pathplan PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Graphviz_INCLUDE_DIR}"
      IMPORTED_LOCATION "${Graphviz_PATHPLAN_LIBRARY}")
  endif ()

  if (NOT TARGET Graphviz::cgraph)
    add_library(Graphviz::cgraph UNKNOWN IMPORTED)
    set_target_properties(Graphviz::cgraph PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Graphviz_INCLUDE_DIR}"
      IMPORTED_LOCATION "${Graphviz_CGRAPH_LIBRARY}"
      INTERFACE_LINK_LIBRARIES "Graphviz::cdt")
  endif ()

  if (NOT TARGET Graphviz::gvc)
    add_library(Graphviz::gvc UNKNOWN IMPORTED)
    set_target_properties(Graphviz::gvc PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Graphviz_INCLUDE_DIR}"
      IMPORTED_LOCATION "${Graphviz_GVC_LIBRARY}"
      INTERFACE_LINK_LIBRARIES "Graphviz::cdt;Graphviz::cgraph;Graphviz::pathplan")
  endif ()
endif()
