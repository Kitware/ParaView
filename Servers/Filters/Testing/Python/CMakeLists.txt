# # pvbatch tests
# SET (PVBATCH_TESTS
#   CTHFragmentConnect
# )
# 
# FOREACH (tfile ${PVBATCH_TESTS})
#   IF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
#     ADD_TEST(${tfile}-Batch
#       ${VTK_MPIRUN_EXE} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS}
#       ${EXECUTABLE_OUTPUT_PATH}/pvbatch
#       ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
#       -D ${PVServerManagerTestData}
#       -T ${ParaView_BINARY_DIR}/Testing/Temporary
#       -V ${PVServerManagerTestData}/Baseline/${tfile}.png
#       -S ${SMSTATE_FILE_DIR})
#   ELSE (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
#     ADD_TEST(${tfile}-Batch
#       ${EXECUTABLE_OUTPUT_PATH}/pvbatch
#       ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
#       -D ${PVServerManagerTestData}
#       -T ${ParaView_BINARY_DIR}/Testing/Temporary
#       -V ${PVServerManagerTestData}/Baseline/${tfile}.png
#       -S ${SMSTATE_FILE_DIR})
#   ENDIF (VTK_MPIRUN_EXE AND VTK_MPI_MAX_NUMPROCS GREATER 1)
# ENDFOREACH (tfile)

