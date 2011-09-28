
# needed if g77 was used
if (UNIX AND PARAVIEW_ENABLE_PYTHON)

  find_file(g2c_library libg2c.so /usr/lib)

  get_filename_component(g2c_library_dir ${g2c_library} PATH)
  get_filename_component(g2c_library_name ${g2c_library} NAME)

  file(GLOB g2c_files "${g2c_library_dir}/${g2c_library_name}*")
  install(PROGRAMS ${g2c_files} 
          DESTINATION ${PV_INSTALL_LIB_DIR}
          COMPONENT Runtime
  )
    
endif (UNIX AND PARAVIEW_ENABLE_PYTHON)
