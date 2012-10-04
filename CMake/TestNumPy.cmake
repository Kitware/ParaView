# This script detects whether numpy is present in the system
EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE}
-c
"
import os
import sys

# prevents dashboard from truncating output of this test.
# print >> sys.stderr, \"Enabling CTEST_FULL_OUTPUT\"
# print >> sys.stderr, \"Checking NUMPY...\"
try:
    import numpy
    numpy.__version__
    # print >> sys.stderr, numpy.__version__
    # print >> sys.stderr, \"----> import numpy WORKS!\"

except ImportError:
    # print >> sys.stderr, \"----> import numpy FAILED.\"
    os._exit(0)

os._exit(1)
"
RESULT_VARIABLE HAS_NUMPY
)

# Doing the HAS_NUMPY string comparison everytime is annoying. Hence setting up
# this FOUND_NUMPY variable.
set (FOUND_NUMPY FALSE)
if ("1" STREQUAL ${HAS_NUMPY})
  set (FOUND_NUMPY TRUE)
endif()
