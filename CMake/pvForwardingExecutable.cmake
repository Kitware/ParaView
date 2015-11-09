#----------------------------------------------------------------------------
# Function for adding an executable with support for shared forwarding.
# Typically, one just uses ADD_EXECUTABLE to add an executable target. However
# on linuxes when rpath is off, and shared libararies are on, to over come the
# need for setting the LD_LIBRARY_PATH, we use shared-forwarding. This macro
# makes it easier to employ shared forwarding if needed.
# ARGUMENTS:
# out_real_exe_suffix -- (out) suffix to be added to the exe-target to locate the
#                     real executable target when shared forwarding is employed.
#                     This is empty when shared forwarding is not needed.
# exe_name        -- (in)  exe target name i.e. the first argument to
#                    ADD_EXECUTABLE.
# Any remaining arguments are simply passed on to the ADD_EXECUTABLE call.
# While writing install rules for this executable. One typically does the
# following.
#   install(TARGETS exe_name
#           DESTINATION "bin"
#           COMPONENT Runtime)
#   if(vtk_exe_suffix)
#     # Shared forwarding enabled.
#     install(TARGETS exe_name${out_real_exe_suffix}
#             DESTINATION "lib"
#             COMPONENT Runtime)
#   endif()
#----------------------------------------------------------------------------
function(pv_add_executable_with_forwarding
         out_real_exe_suffix
         exe_name)
  if(NOT DEFINED PV_INSTALL_LIBRARY_DIR)
    message(FATAL_ERROR
      "PV_INSTALL_LIBRARY_DIR variable must be set before calling add_executable_with_forwarding")
  endif()

  pv_add_executable_with_forwarding2(out_var "" ""
    ${PV_INSTALL_LIBRARY_DIR}
    ${exe_name} ${ARGN})
  set(${out_real_exe_suffix} "${out_var}" PARENT_SCOPE)
endfunction()

#----------------------------------------------------------------------------
function(pv_add_executable_with_forwarding2
         out_real_exe_suffix
         extra_build_dirs
         extra_install_dirs
         install_lib_dir
         exe_name)

  set(mac_bundle)
  if(APPLE)
    set(largs ${ARGN})
    list(FIND largs "MACOSX_BUNDLE" mac_bundle_index)
    if(mac_bundle_index GREATER -1)
      set(mac_bundle TRUE)
    endif()
  endif()

  set(PV_EXE_SUFFIX)
  if(BUILD_SHARED_LIBS AND NOT mac_bundle)
    if(NOT WIN32)
      set(exe_output_path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
      if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(exe_output_path ${CMAKE_BINARY_DIR})
      endif()
      set(PV_EXE_SUFFIX -launcher)
      set(PV_FORWARD_DIR_BUILD "${exe_output_path}")
      set(PV_FORWARD_DIR_INSTALL "../${install_lib_dir}")
      set(PV_FORWARD_PATH_BUILD "\"${PV_FORWARD_DIR_BUILD}\"")
      set(PV_FORWARD_PATH_INSTALL "\"${PV_FORWARD_DIR_INSTALL}\"")
      foreach(dir ${extra_build_dirs})
        set(PV_FORWARD_PATH_BUILD "${PV_FORWARD_PATH_BUILD},\"${dir}\"")
      endforeach()
      foreach(dir ${extra_install_dirs})
        set(PV_FORWARD_PATH_INSTALL "${PV_FORWARD_PATH_INSTALL},\"${dir}\"")
      endforeach()

      set(PV_FORWARD_EXE ${exe_name})
      configure_file(
        ${ParaView_CMAKE_DIR}/pv-forward.c.in
        ${CMAKE_CURRENT_BINARY_DIR}/${exe_name}-forward.c
        @ONLY)
      add_executable(${exe_name}${PV_EXE_SUFFIX}
        ${CMAKE_CURRENT_BINARY_DIR}/${exe_name}-forward.c)
      set_target_properties(${exe_name}${PV_EXE_SUFFIX} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/launcher)
      set_target_properties(${exe_name}${PV_EXE_SUFFIX} PROPERTIES
        OUTPUT_NAME ${exe_name})
      add_dependencies(${exe_name}${PV_EXE_SUFFIX} ${exe_name})
    endif()
  endif()

  add_executable(${exe_name} ${ARGN})

  set(${out_real_exe_suffix} "${PV_EXE_SUFFIX}" PARENT_SCOPE)
endfunction()
