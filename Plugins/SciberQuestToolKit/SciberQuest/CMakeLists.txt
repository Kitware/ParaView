#--------------------------------------------------
# Configuration options
#--------------------------------------------------
find_package(CUDA)
include(CMakeDependentOption)
cmake_dependent_option(SQTK_CUDA "Enable CUDA optimizations." ON
  CUDA_FOUND OFF)
mark_as_advanced(SQTK_CUDA)

option(SQTK_DEBUG "Enable debug output to stderr." OFF)
mark_as_advanced(SQTK_DEBUG)
if (SQTK_DEBUG)
  add_definitions(-DSQTK_DEBUG)
endif()

include(EigenConfig.cmake)

set(SCIBER_QUEST_SOURCES
  BOVMetaData.cxx
  BOVReader.cxx
  BOVScalarImage.cxx
  BOVTimeStepImage.cxx
  BOVVectorImage.cxx
  BOVWriter.cxx
  CartesianBounds.cxx
  CartesianDataBlock.cxx
  CartesianDataBlockIODescriptor.cxx
  CartesianDataBlockIODescriptorIterator.cxx
  CartesianDecomp.cxx
  CartesianExtent.cxx
  CartesianExtentIterator.cxx
  CellCopier.cxx
  CellIdIterator.cxx
  CPUConvolutionDriver.cxx
  CUDAConstMemoryManager.hxx
  CUDAConvolutionDriver.cxx
  FieldDisplacementMapData.cxx
  FieldLine.cxx
  FieldTopologyMapData.cxx
  FieldTraceData.cxx
  FlatIndex.cxx
  FsUtils.cxx
  GDAMetaData.cxx
  GDAMetaDataKeys.cxx
  GhostTransaction.cxx
  IdBlock.cxx
  ImageDecomp.cxx
  IntersectionSet.cxx
  LogBuffer.cxx
  MemOrder.hxx
  MemoryMonitor.cxx
  PoincareMapData.cxx
  PolyDataCellCopier.cxx
  PolyDataFieldDisplacementMap.cxx
  PolyDataFieldTopologyMap.cxx
  postream.cxx
  PrintUtils.cxx
  RectilinearDecomp.cxx
  RefCountedPointer.cxx
  StreamlineData.cxx
  TerminationCondition.cxx
  TopologicalClassSelector.cxx
  UnstructuredFieldDisplacementMap.cxx
  UnstructuredFieldTopologyMap.cxx
  UnstructuredGridCellCopier.cxx
  XMLUtils.cxx
  )
if (WIN32)
  list(APPEND SCIBER_QUEST_SOURCES
    win_windirent.cxx)
endif()

set(CXX_SOURCES
  ${SCIBER_QUEST_SOURCES}
  vtkSQAgyrotropyFilter.cxx
  vtkSQBOVMetaReader.cxx
  vtkSQBOVReader.cxx
  vtkSQBOVReaderBase.cxx
  vtkSQBOVWriter.cxx
  vtkSQBinaryThreshold.cxx
  vtkSQCellGenerator.cxx
  vtkSQEdgeFilter.cxx
  vtkSQFTLE.cxx
  vtkSQFieldTopologySelect.cxx
  vtkSQFieldTopologySplit.cxx
  vtkSQFieldTracer.cxx
  vtkSQHemisphereSource.cxx
  vtkSQImageGhosts.cxx
  vtkSQImageSource.cxx
  vtkSQKernelConvolution.cxx
  vtkSQLineSource.cxx
  vtkSQLog.cxx
  vtkSQLogSource.cxx
  vtkSQMedianFilter.cxx
  vtkSQMetaDataKeys.cxx
  vtkSQOOCBOVReader.cxx
  vtkSQOOCReader.cxx
  vtkSQPlaneSource.cxx
  vtkSQPlaneSourceCellGenerator.cxx
  vtkSQPointSource.cxx
  vtkSQRandomCells.cxx
  vtkSQRandomSeedPoints.cxx
  vtkSQSeedPointLatice.cxx
  vtkSQSphereSource.cxx
  vtkSQSurfaceVectors.cxx
  vtkSQTubeFilter.cxx
  vtkSQVolumeSource.cxx
  vtkSQVolumeSourceCellGenerator.cxx
  vtkSQVortexFilter.cxx
  )

if (CUDA_FOUND AND SQTK_CUDA)
  #message(STATUS "SQTK Including CUDA accelerated filters.")
  #message(STATUS "SQTK Cuda ${CUDA_VERSION_STRING} found.")
  set(CUDA_NVCC_FLAGS "-arch=sm_20;--compiler-options;-fPIC" CACHE STRING "nvcc flags")
  set(CUDA_NVCC_DEBUG_FLAGS "-g;-G;" CACHE STRING "nvcc debug flags")
  set(CUDA_NVCC_RELEASE_FLAGS "-O3" CACHE STRING "nvcc release flags")
  #message(STATUS "SQTK CUDA_NVCC_FLAGS=${CUDA_NVCC_FLAGS}")
  #message(STATUS "SQTK CUDA_NVCC_DEBUG_FLAGS=${CUDA_NVCC_DEBUG_FLAGS}")
  #message(STATUS "SQTK CUDA_NVCC_RELEASE_FLAGS=${CUDA_NVCC_FLAGS}")
  set(CU_SOURCES CUDAConvolutionDriver.cu)
  set_property(
    SOURCE ${CU_SOURCES}
    APPEND PROPERTY COMPILE_DEFINITIONS SQTK_CUDA
    )
  cuda_add_library(CUDAConvolutionDriver ${CU_SOURCES})
  install(TARGETS CUDAConvolutionDriver DESTINATION lib/paraview-${PARAVIEW_VERSION_MAJOR}-${PARAVIEW_VERSION_MINOR})
  cuda_build_clean_target()
else()
  list(APPEND CXX_SOURCES
    CUDAConvolutionDriver.cxx
    )
endif()

set_source_files_properties(
  vtkSQBOVReaderBase.cxx
  vtkSQOOCReader.cxx
  vtkSQCellGenerator.cxx
  ABSTRACT
  )

set_source_files_properties(
  vtkSQOOCReader.cxx
  vtkSQOOCBOVReader.cxx
  vtkSQMetaDataKeys.cxx
  ${SCIBER_QUEST_SOURCES}
  WRAP_EXCLUDE
  )

vtk_module_library(vtkSciberQuest ${CXX_SOURCES})

if (SQTK_CUDA AND CUDA_FOUND)
  target_link_libraries(vtkSciberQuest
  LINK_PUBLIC CUDAConvolutionDriver)
endif()

install(TARGETS vtkSciberQuest DESTINATION lib/paraview-${PARAVIEW_VERSION_MAJOR}-${PARAVIEW_VERSION_MINOR})

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
  vtk_mpi_link(vtkSciberQuest)
endif()
