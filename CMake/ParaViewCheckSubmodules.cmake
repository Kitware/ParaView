# This is where the logic resides for verifying the source tree layout.
# This is necessary due to the moves made in Titan's source tree.

function(CheckGitDirectory path)
  # Emit a fatal error and inform the user to init their submodules.
  if(NOT EXISTS "${path}/.git/config")
    message(FATAL_ERROR "
 Please initialize the git submodules.
 ${path} is not a valid git submodule.
 --
 Run the following commands to initialize the ParaView Git submodules.
 cd ${ParaView_SOURCE_DIR}
 git submodule update --init
 ")
  endif()
endfunction(CheckGitDirectory)

set(ParaView_Submodules VTK Utilities/IceT Utilities/Xdmf2)

foreach(submodule ${ParaView_Submodules})
  # If this is a git checkout, then check the submodules were initialized.
  if(EXISTS "${ParaView_SOURCE_DIR}/.git/config")
    CheckGitDirectory("${ParaView_SOURCE_DIR}/${submodule}")
  endif()
endforeach()
