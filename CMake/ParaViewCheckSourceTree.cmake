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

# Emit a fatal error and inform the user if they have not enabled hooks.
option(ParaView_IGNORE_HOOKS "Should the ParaView hooks check be ignored?" OFF)
option(VTK_IGNORE_HOOKS "Should the VTK hooks check be ignored?" OFF)
if(DEFINED ENV{DASHBOARD_TEST_FROM_CTEST})
  set(ParaView_FROM_CTEST TRUE)
endif()

if(NOT VTK_IGNORE_HOOKS AND NOT ParaView_FROM_CTEST AND
    EXISTS "${VTK_SOURCE_DIR}/.git/config")
  # Now check the VTK local hooks - mostly a convenience to avoid two failures.
  if(NOT EXISTS "${VTK_SOURCE_DIR}/.git/hooks/.git/config")
    set(VTK_GIT_HOOKS
"cd ${VTK_SOURCE_DIR}/.git/hooks
 git init
 git pull .. remotes/origin/hooks")

    set(VTK_GIT_HOOKS2 "
 If you wish to ignore this check for a build set the CMake cache variable
 VTK_IGNORE_HOOKS to ON. To ignore this check in all builds either archive
 your clone, or create the file ${VTK_SOURCE_DIR}/.git/hooks/.git/config
 in your source tree.
 ")
  endif()
endif()
if(NOT ParaView_IGNORE_HOOKS AND NOT ParaView_FROM_CTEST AND
    EXISTS "${ParaView_SOURCE_DIR}/.git/config")
  if(NOT EXISTS "${ParaView_SOURCE_DIR}/.git/hooks/.git/config")
    message(FATAL_ERROR "
 Please initialize your local Git hooks, paste the following into a shell:

 cd ${ParaView_SOURCE_DIR}/.git/hooks
 git init
 git pull .. remotes/origin/hooks
 ${VTK_GIT_HOOKS}
 cd ${ParaView_SOURCE_DIR}

 See http://www.vtk.org/Wiki/VTK/Git#Hooks for more details.

 If you wish to ignore this check for a build set the CMake cache variable
 ParaView_IGNORE_HOOKS to ON. To ignore this check in all builds either archive
 your clone, or create the file ${ParaView_SOURCE_DIR}/.git/hooks/.git/config
 in your source tree.
 ${VTK_GIT_HOOKS2}")
  endif()
endif()
