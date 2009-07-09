PROJECT(mpi4py)

IF (PARAVIEW_ENABLE_PYTHON AND PARAVIEW_USE_MPI)

  SET(PV_MPI_PYTHON_MODULE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mpi")
  SET(PV_MPI_PYTHON_MODULE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/mpi")
  MAKE_DIRECTORY(${PV_MPI_PYTHON_MODULE_BINARY_DIR})

  # Handle out-of-source builds correctly.
  #
  #  1. Create a list of Python files to be installed/copied.
  #  2. Copy them to the binary directory. Since paraview cannot be built
  #     in place, we must always copy the files to the binary directory.
  #  3. Use Python's compileall to compile the copied files.
  #
  # *** Step 1 has to be done carefully to avoid missing out files ***


  # List all python source files.
  # All files paraview/*.py included in the paraview module.
  SET(PV_MPI_PYTHON_FILES
    include/mpi4py/MPI.pxd
    include/mpi4py/__init__.pyx
    include/mpi4py/mpi.pxi
    include/mpi4py/mpi4py.h
    include/mpi4py/mpi4py.i
    include/mpi4py/mpi4py_MPI.h
    include/mpi4py/mpi4py_MPI_api.h
    include/mpi4py/mpi_c.pxd
    __init__.py
    rc.py
    )

  # Now copy these files if necessary.
  SET(PV_MPI_PYTHON_SOURCE_FILES)
  SET(PV_MPI_PYTHON_OUTPUT_FILES)
  FOREACH(file ${PV_MPI_PYTHON_FILES})
    SET(src "${PV_MPI_PYTHON_MODULE_SOURCE_DIR}/${file}")
    SET(PV_MPI_PYTHON_SOURCE_FILES ${PV_MPI_PYTHON_SOURCE_FILES} ${src})
  ENDFOREACH(file)

  FOREACH(file ${PV_MPI_PYTHON_FILES})
    SET(src "${PV_MPI_PYTHON_MODULE_SOURCE_DIR}/${file}")
    SET(tgt "${PV_MPI_PYTHON_MODULE_BINARY_DIR}/${file}")
    ADD_CUSTOM_COMMAND(DEPENDS ${src}
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy ${src} ${tgt}
      OUTPUT ${tgt}
      COMMENT "source copy")
    SET(PV_MPI_PYTHON_OUTPUT_FILES ${PV_MPI_PYTHON_OUTPUT_FILES} ${tgt})
  ENDFOREACH(file)

  # Compile and create MPI.so in PV_MPI_PYTHON_MODULE_BINARY_DIR
  ADD_SUBDIRECTORY("${CMAKE_CURRENT_SOURCE_DIR}/Executable")
  
  # Byte compile the Python files.
  CONFIGURE_FILE(${PV_MPI_PYTHON_MODULE_SOURCE_DIR}/compile_all_pv_mpi.py.in
    ${PV_MPI_PYTHON_MODULE_BINARY_DIR}/compile_all_pv_mpi.py
    @ONLY IMMEDIATE)

  ADD_CUSTOM_COMMAND(
    WORKING_DIRECTORY ${PV_MPI_PYTHON_MODULE_BINARY_DIR}
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS compile_all_pv_mpi.py
    DEPENDS ${PV_MPI_PYTHON_SOURCE_FILES} ${PV_MPI_PYTHON_MODULE_BINARY_DIR}/compile_all_pv_mpi.py
    ${PV_MPI_PYTHON_OUTPUT_FILES} ${PV_MPI_PYTHON_MODULE_BINARY_DIR}/MPI.so
    OUTPUT "${PV_MPI_PYTHON_MODULE_BINARY_DIR}/pv_mpi_compile_complete"
    )
  
  ADD_CUSTOM_TARGET(paraview_mpi_pyc ALL 
    DEPENDS "${PV_MPI_PYTHON_MODULE_BINARY_DIR}/pv_mpi_compile_complete")

  ADD_DEPENDENCIES(paraview_mpi_pyc paraview_mpi4py_pyc)
  
  # Install the paraview module files.
  IF (NOT PV_INSTALL_NO_LIBRARIES)
    INSTALL(DIRECTORY ${PV_MPI_PYTHON_MODULE_BINARY_DIR} 
        DESTINATION ${PV_INSTALL_LIB_DIR}
        COMPONENT Runtime
        )
  ENDIF (NOT PV_INSTALL_NO_LIBRARIES)
ENDIF (PARAVIEW_ENABLE_PYTHON AND PARAVIEW_USE_MPI)
