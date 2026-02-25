find_package(GenericIO REQUIRED)
find_package(Threads REQUIRED)

set(classes
    BHTree
    bigchunk
    ChainingMesh
    CosmoHaloFinder
    CosmoHaloFinderP
    dims
    FOFDistribute
    FOFHaloProperties
    GridExchange
    HaloCenterFinder
    Message
    ParticleDistribute
    ParticleExchange
    Partition
    SimpleTimings
    SODHalo
    SubHaloFinder)

set(headers
  BasicDefinition.h
  CosmoHalo.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkCosmoHaloFinderModule.h")

set(private_headers
  Timer.h
  Timings.h)

vtk_module_add_module(ParaView::cosmohalofinder
  CLASSES ${classes}
  HEADERS ${headers}
  PRIVATE_HEADERS ${private_headers}
  HEADERS_SUBDIR "vtkcosmohalofinder")
vtk_module_link(ParaView::cosmohalofinder
  PRIVATE
    ${GENERIC_IO_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})
vtk_module_include(ParaView::cosmohalofinder SYSTEM
  INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkcosmohalofinder>"
  PRIVATE
    ${GENERIC_IO_INCLUDE_DIR})

include(GenerateExportHeader)
generate_export_header(vtkcosmohalofinder
  EXPORT_MACRO_NAME VTKCOSMOHALOFINDER_EXPORT
  EXPORT_FILE_NAME  "vtkCosmoHaloFinderModule.h")
