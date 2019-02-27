if (USE_CATALYST)
  add_library(CxxParticlePathExampleAdaptor
    FEAdaptor.cxx
    FEAdaptor.h)
  target_link_libraries(CxxParticlePathExampleAdaptor
    PRIVATE
      ParaView::PythonCatalyst
      VTK::CommonDataModel
      VTK::ParallelMPI)
endif ()

add_executable(CxxParticlePathExample
  FEDataStructures.cxx
  FEDataStructures.h
  FEDriver.cxx)
if (TARGET CxxParticlePathExampleAdaptor)
  target_link_libraries(CxxParticlePathExample
    PRIVATE
      CxxParticlePathExampleAdaptor
      VTK::mpi)
else ()
  target_link_libraries(CxxParticlePathExample
    PRIVATE
      MPI::MPI_C)
endif ()

if (TARGET ParaView::pvbatch AND BUILD_TESTING)
  add_test(NAME CxxParticlePathExampleTest
    COMMAND ${CMAKE_COMMAND}
    -DCOPROCESSING_TEST_DRIVER:FILEPATH=$<TARGET_FILE:CxxParticlePathExample>
    -DCOPROCESSING_TEST_DIR:PATH=${CMAKE_BINARY_DIR}/Testing/Temporary
    -DCOPROCESSING_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DPVBATCH:FILEPATH=$<TARGET_FILE:ParaView::pvbatch>
    -DMPIEXEC:FILEPATH=${MPIEXEC_EXECUTABLE}
    -DMPIEXEC_NUMPROC_FLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
    -DMPIEXEC_NUMPROCS=4
    -DMPIEXEC_PREFLAGS:STRING=${MPIEXEC_PREFLAGS}
    -DMPIEXEC_POSTFLAGS:STRING=${MPIEXEC_POSTFLAGS}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/ParticlePathTesting.cmake)
endif ()
