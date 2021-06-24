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

if(Graphviz_INCLUDE_DIR
  AND Graphviz_CDT_LIBRARY
  AND Graphviz_GVC_LIBRARY
  AND Graphviz_CGRAPH_LIBRARY
  AND Graphviz_PATHPLAN_LIBRARY
)
  set(Graphviz_FOUND TRUE)
else()
  set(Graphviz_FOUND FALSE)
endif()
