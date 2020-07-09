enable_language(Fortran)
include(FortranCInterface)

FortranCInterface_HEADER(PagosaAdaptorAPIMangling.h SYMBOLS
  setcoprocessorgeometry setcoprocessorfield
  setmarkergeometry
  setmarkerscalarfield setmarkervectorfield setmarkertensorfield)

set(headers
  PagosaAdaptor.h
)

set(private_headers
  "${CMAKE_CURRENT_BINARY_DIR}/PagosaAdaptorAPIMangling.h"
)

set(sources
  PagosaAdaptor.cxx
)

vtk_module_add_module(ParaView::AdaptorsPagosa
  HEADERS           ${header}
  PRIVATE_HEADERS   ${private_headers}
  SOURCES           ${sources}
)
vtk_module_client_server_exclude()
