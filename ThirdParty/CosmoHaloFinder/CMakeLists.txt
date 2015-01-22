include(vtkMPI)

set(${vtk-module}_SYSTEM_INCLUDE_DIRS
    ${MPI_C_INCLUDE_PATH}
    ${GENERIC_IO_INCLUDE_DIR}
)

find_package(GenericIO REQUIRED)
find_package(Threads REQUIRED)

set (${vtk-module}_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/CosmoHaloFinderP.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/Timings.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/Timer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SubHaloFinder.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/SubHaloFinderDev.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/SUBFIND.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SODHalo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleTimings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Partition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ParticleExchange.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ParticleDistribute.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Message.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/InitialExchange.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/HaloFinderInput.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HaloCenterFinder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GridExchange.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FOFHaloProperties.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FOFDistribute.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/dims-local-init.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dims.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Definition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CosmoHaloFinderP.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CosmoHalo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CosmoHaloFinder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ChainingMesh.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bigchunk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BHTree.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BasicDefinition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CosmoToolsDefinitions.h
    ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}Module.h
)

include_directories(
    ${GENERIC_IO_INCLUDE_DIR}
)

set(Module_SRCS
    BHTree.cxx
    bigchunk.cxx
    ChainingMesh.cxx
    CMakeLists.txt
    CosmoHaloFinder.cxx
    CosmoHaloFinderP.cxx
    dims.cxx
#    dims-local-init.cxx
    FOFDistribute.cxx
    FOFHaloProperties.cxx
    GridExchange.cxx
    HaloCenterFinder.cxx
#    HaloFinderInput.cxx
#    InitialExchange.cxx
    Message.cxx
    ParticleDistribute.cxx
    ParticleExchange.cxx
    Partition.cxx
    SimpleTimings.cxx
    SODHalo.cxx
#    SUBFIND.cxx
    SubHaloFinder.cxx
#    SubHaloFinderDev.cxx
#    Timer.cxx
#    Timings.cxx
)

vtk_module_library(${vtk-module} ${Module_SRCS})

target_link_libraries(${vtk-module} LINK_PRIVATE
                          ${GENERIC_IO_LIBRARIES}
                          ${CMAKE_THREAD_LIBS_INIT})
vtk_mpi_link(${vtk-module})
