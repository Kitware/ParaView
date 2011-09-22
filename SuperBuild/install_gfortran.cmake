
if (UNIX AND PARAVIEW_ENABLE_PYTHON)

  find_library(gfortran_library libgfortran.so)

  get_filename_component(gfortran_library_dir ${gfortran_library} PATH)
  get_filename_component(gfortran_library_name ${gfortran_library} NAME)

  install(DIRECTORY ${gfortran_library_dir}/
        DESTINATION ${PV_INSTALL_LIB_DIR} COMPONENT Runtime
        FILES_MATCHING PATTERN "${gfortran_library_name}*")
    
endif (UNIX AND PARAVIEW_ENABLE_PYTHON)
