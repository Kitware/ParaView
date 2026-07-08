if (NOT VTK_MODULE_USE_EXTERNAL_VTK_zlib)
    message(FATAL_ERROR
      "DPvz requires VTK_MODULE_USE_EXTERNAL_VTK_zlib")
endif()

if (TARGET VTK::mpi)
  # find the MPI version of DPvz
  set(DPVZ_MPI 1)
else()
  set(DPVZ_SER 1)
endif ()

vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE DPvz)

set(generated_headers
  vtkDPvz.h
)

foreach (generated_header IN LISTS generated_headers)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${generated_header}.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${generated_header}"
    @ONLY)
  list(APPEND headers
    "${CMAKE_CURRENT_BINARY_DIR}/${generated_header}")
endforeach ()

set(classes
  vtkDPvtkMultiBlockDataReader)

vtk_module_add_module(DPvtkReader::vtkDPvtkReader
  HEADERS ${headers}
  CLASSES ${classes})
vtk_module_link(DPvtkReader::vtkDPvtkReader
  NO_KIT_EXPORT_IF_SHARED
  PUBLIC
    DPvz::DPvz
)

paraview_add_server_manager_xmls(
  XMLS vtkDPvtkReader.xml)
