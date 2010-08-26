# Try to identify the current development source version.
set(ParaView_SOURCE_VERSION "")
if(EXISTS ${ParaView_SOURCE_DIR}/.git/HEAD)
  find_program(GIT_EXECUTABLE NAMES git git.cmd)
  mark_as_advanced(GIT_EXECUTABLE)
  if(GIT_EXECUTABLE)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --verify -q --short=4 HEAD
      OUTPUT_VARIABLE head
      OUTPUT_STRIP_TRAILING_WHITESPACE
      WORKING_DIRECTORY ${ParaView_SOURCE_DIR}
      )
    if(head)
      set(ParaView_SOURCE_VERSION "g${head}")
      execute_process(
        COMMAND ${GIT_EXECUTABLE} update-index -q --refresh
        WORKING_DIRECTORY ${ParaView_SOURCE_DIR}
        )
      execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-index --name-only HEAD --
        OUTPUT_VARIABLE dirty
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY ${ParaView_SOURCE_DIR}
        )
      if(dirty)
        #set(ParaView_SOURCE_VERSION "${ParaView_SOURCE_VERSION}-dirty")
      endif()
    endif()
  endif()
endif()