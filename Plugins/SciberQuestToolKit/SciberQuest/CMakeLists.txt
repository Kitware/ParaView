#--------------------------------------------------
# Configuration options
#--------------------------------------------------


include(EigenConfig.cmake)
include(CUDAConfig.cmake)

set(SQTK_CXX_SOURCES
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
  list(APPEND SQTK_CXX_SOURCES
    win_windirent.cxx)
endif()

if (SQTK_CUDA AND CUDA_FOUND)
  set(CU_SOURCES CUDAConvolutionDriver.cu)
  set_property(
    SOURCE ${CU_SOURCES}
    APPEND PROPERTY COMPILE_DEFINITIONS SQTK_CUDA)
  cuda_include_directories(${CMAKE_CURRENT_BINARY_DIR} ${VTK_INCLUDE_DIRS})
  cuda_add_library(CUDAConvolutionDriver ${CU_SOURCES})
  vtk_target_export(CUDAConvolutionDriver)
  cuda_build_clean_target()
  install(TARGETS CUDAConvolutionDriver
    ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR}
    LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR})
else()
  # non-cuda version of CUDAConvolutionDriver.cu
  set(SQTK_CXX_SOURCES ${SQTK_CXX_SOURCES} CUDAConvolutionDriver.cxx)
endif()

set(CXX_SOURCES
  ${SQTK_CXX_SOURCES}
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
  ${SQTK_CXX_SOURCES}
  WRAP_EXCLUDE
  )

option(SQTK_DEBUG "Enable debug output to stderr." OFF)
mark_as_advanced(SQTK_DEBUG)
if (SQTK_DEBUG)
  add_definitions(-DSQTK_DEBUG)
endif()

vtk_module_library(vtkSciberQuest ${CXX_SOURCES})

if (SQTK_CUDA AND CUDA_FOUND)
  target_link_libraries(vtkSciberQuest
    LINK_PRIVATE ${CUDA_LIBRARIES} CUDAConvolutionDriver
    )
endif()

install(TARGETS vtkSciberQuest
   ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR}
   LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR})

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
  vtk_mpi_link(vtkSciberQuest)
endif()
