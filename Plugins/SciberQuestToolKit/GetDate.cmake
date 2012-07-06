#    ____    _ __           ____               __    ____
#   / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
#  _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
# /___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
# Copyright 2012 SciberQuest Inc.
#
macro (GetDate RESULT)
  set(${RESULT} "00/00/0000")
  if (WIN32)
    execute_process(COMMAND "cmd" " /C date /T" OUTPUT_VARIABLE ${RESULT})
  elseif (UNIX)
    execute_process(COMMAND "date" "+%m/%d/%Y" OUTPUT_VARIABLE ${RESULT})
  endif ()
  string(REGEX REPLACE "(.*)(\n|\r)$" "\\1" ${RESULT} ${${RESULT}})
endmacro ()
