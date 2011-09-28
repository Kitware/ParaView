
if (UNIX AND PARAVIEW_ENABLE_PYTHON)

  find_library(gfortran_library libgfortran.so NAMES libgfortran.so.1)

  get_filename_component(gfortran_library_dir ${gfortran_library} PATH)
  get_filename_component(gfortran_library_name ${gfortran_library} NAME)

  file(GLOB gfortran_files "${gfortran_library_dir}/${gfortran_library_name}*")
  install(PROGRAMS ${gfortran_files} 
          DESTINATION ${PV_INSTALL_LIB_DIR}
          COMPONENT Runtime
  )
    
endif (UNIX AND PARAVIEW_ENABLE_PYTHON)
