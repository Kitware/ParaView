
if (UNIX AND PARAVIEW_ENABLE_PYTHON)

  find_file(bz2_library NAMES libbz2 libbz2.so libbz2.so.1 PATHS /lib /lib64)

  get_filename_component(bz2_library_dir ${bz2_library} PATH)
  get_filename_component(bz2_library_name ${bz2_library} NAME)

  file(GLOB bz2_files "${bz2_library_dir}/${bz2_library_name}*")
  install(PROGRAMS ${bz2_files} 
          DESTINATION ${PV_INSTALL_LIB_DIR}
          COMPONENT Runtime
  )
    
endif (UNIX AND PARAVIEW_ENABLE_PYTHON)
