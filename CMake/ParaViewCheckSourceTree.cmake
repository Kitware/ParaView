# This is where the logic resides for verifying the source tree layout.

function(CheckGitDirectory path submodule)
  # Emit a fatal error and inform the user to init their submodules.
  if(NOT EXISTS "${path}/${submodule}/.git")
    message(FATAL_ERROR "
 Please initialize the git submodules.
 ${path} is not a valid git submodule.
 --
 Run the following commands to initialize the ParaView Git submodules.
 cd ${ParaView_SOURCE_DIR}
 git submodule update --init
 ")
  endif()
endfunction()

set(ParaView_Submodules VTK Utilities/IceT Utilities/Xdmf2 Qt/Testing)

foreach(submodule ${ParaView_Submodules})
  # If this is a git checkout, then check the submodules were initialized.
  if(EXISTS "${ParaView_SOURCE_DIR}/.git/config")
    CheckGitDirectory("${ParaView_SOURCE_DIR}" "${submodule}")
  endif()
endforeach()

# Install a pre-commit hook to bootstrap commit hooks.
if(EXISTS "${ParaView_SOURCE_DIR}/.git/config" AND
    NOT EXISTS "${ParaView_SOURCE_DIR}/.git/hooks/pre-commit")
  # Silently ignore the error if the hooks directory is read-only.
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy ${ParaView_SOURCE_DIR}/CMake/pre-commit
                                     ${ParaView_SOURCE_DIR}/.git/hooks/pre-commit
    OUTPUT_VARIABLE _output
    ERROR_VARIABLE  _output
    RESULT_VARIABLE _result
    )
  if(_result AND NOT "${_output}" MATCHES "Error copying file")
    message("${_output}")
  endif()
endif()
