SET(PV_MPI4PY_EXEC_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
SET(PV_MPI4PY_EXEC_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

MAKE_DIRECTORY(${PV_MPI4PY_EXEC_BINARY_DIR})
# Handle out-of-source builds correctly.
#
#  1. Create a list of files to be installed/copied.
#  2. Copy them to the binary directory. Since paraview cannot be built
#     in place, we must always copy the files to the binary directory.
#  3. Compile the file and place it in ${PV_MPI4PY_EXEC_BINARY_DIR}../mpi directory.
#
# *** Step 1 has to be done carefully to avoid missing out files ***


# List all python source files.
# All files paraview/*.py included in the paraview module.
SET(PV_MPI4PY_EXEC_FILES
  atimport.h
  missing.h
  mpi4py_MPI.c
  mpi4py.MPI.pyx
  MPI.c
  python.c
  vendor.h
  compat/anympi.h
  compat/deinompi.h
  compat/lammpi.h
  compat/mpich1.h
  compat/mpich2.h
  compat/openmpi.h
  compat/sgimpi.h
  config/deinompi.h
  config/lammpi.h
  config/lammpiio.h
  config/mpich1.h
  config/mpich1io.h
  config/mpich2.h
  config/mpich2io.h
  config/openmpi.h
  config/sgimpi.h
  MPI/allocate.pxi
  MPI/asbuffer.pxi
  MPI/asmpistr.pxi
  MPI/CAPI.pxi
  MPI/Datatype.pyx
  MPI/ExceptionC.pyx
  MPI/Exception.pyx
  MPI/Group.pyx
  MPI/Info.pyx
  MPI/MPI.pyx
  MPI/Op.pyx
  MPI/reqimpl.pxi
  MPI/Status.pyx
  MPI/winimpl.pxi
  MPI/asarray.pxi
  MPI/asmemory.pxi
  MPI/atimport.pxi
  MPI/Comm.pyx
  MPI/Errhandler.pyx
  MPI/ExceptionP.pyx
  MPI/File.pyx
  MPI/helpers.pxi
  MPI/message.pxi
  MPI/opimpl.pxi
  MPI/pickled.pxi
  MPI/Request.pyx
  MPI/stdlib.pxi
  MPI/Win.pyx)

# Now copy these files if necessary.
SET(PV_MPI4PY_SOURCE_FILES)
SET(PV_MPI4PY_OUTPUT_FILES)
FOREACH(file ${PV_MPI4PY_EXEC_FILES})
  SET(src "${PV_MPI4PY_EXEC_SOURCE_DIR}/${file}")
  SET(PV_MPI4PY_SOURCE_FILES ${PV_MPI4PY_SOURCE_FILES} ${src})
ENDFOREACH(file)

FOREACH(file ${PV_MPI4PY_EXEC_FILES})
  SET(src "${PV_MPI4PY_EXEC_SOURCE_DIR}/${file}")
  SET(tgt "${PV_MPI4PY_EXEC_BINARY_DIR}/${file}")
  ADD_CUSTOM_COMMAND(DEPENDS ${src}
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${src} ${tgt}
    OUTPUT ${tgt}
    COMMENT "source copy")
  SET(PV_MPI4PY_OUTPUT_FILES ${PV_MPI4PY_OUTPUT_FILES} ${tgt})
ENDFOREACH(file)

ADD_CUSTOM_TARGET(paraview_mpi4py_pyc ALL
  COMMAND ${MPI_C_COMPILER} -fno-strict-aliasing -DNDEBUG -g -fwrapv -O2 -Wall -Wstrict-prototypes -fPIC -I${PYTHON_INCLUDE_PATH} -pthread -shared -Wl,-O1 -Wl,-Bsymbolic-functions MPI.c -o ${PV_MPI_PYTHON_MODULE_BINARY_DIR}/MPI.so
  WORKING_DIRECTORY ${PV_MPI4PY_EXEC_BINARY_DIR}
  DEPENDS ${PV_MPI4PY_OUTPUT_FILES}
  COMMENT "Building MPI.so for mpi4pi")
  
