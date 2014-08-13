# Script used to freeze Python
# Usage:
#   cmake
#       -DOUTPUT_DIRECTORY:PATH="..."
#       -DPACKAGE_ROOT:PATH="..."
#       -DPYTHON_EXECUTABLE:FILEPATH="Python executable"
#       -DOUTPUT_HEADER_PREFIX:STRING"..."
#       -P vtkFreezePython.cmake
# Example:
# cmake -DOUTPUT_HEADER_PREFIX:STRING=FrozenPython
#       -DOUTPUT_DIRECTORY:PATH=/tmp/frozen_paraview
#       -DPACKAGE_ROOT=/home/utkarsh/Kitware/ParaView3/ParaViewBin/lib/site-packages
#       -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/python
#       -P ThirdParty/FreezePython/vtkFreezePython.cmake

if (NOT OUTPUT_DIRECTORY)
    message(FATAL_ERROR "No OUTPUT_DIRECTORY specified.")
endif()
if (NOT PACKAGE_ROOT)
    message(FATAL_ERROR "No PACKAGE_ROOT specified.")
endif()
if (NOT IS_DIRECTORY "${PACKAGE_ROOT}")
    message(FATAL_ERROR "PACKAGE_ROOT must be a directory.")
endif()
if (NOT PYTHON_EXECUTABLE)
    message(FATAL_ERROR "PYTHON_EXECUTABLE must be specified.")
endif()
if (NOT OUTPUT_HEADER_PREFIX)
    message(FATAL_ERROR "OUTPUT_HEADER_PREFIX must be specified.")
endif()

# cleanup output directory.
message("Removing ${OUTPUT_DIRECTORY}")
file(REMOVE_RECURSE "${OUTPUT_DIRECTORY}")
file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")
set(ENV{PYTHONPATH} "${PACKAGE_ROOT}")
execute_process(
    COMMAND ${PYTHON_EXECUTABLE}
            "${CMAKE_CURRENT_LIST_DIR}/freeze/freeze_paraview.py"
            -o "${OUTPUT_DIRECTORY}"
            -X distutils
            -X locale
            -X macpath
            -X matplotlib
            -X ntpath
            -X os2emxpath
            -X popen2
            -X pydoc
            -X readline
            -X setuptools
            -X _warnings

            -X zope
            -X twisted
            -X autobahn

            "${CMAKE_CURRENT_LIST_DIR}/dummy.py"
            -p "${PACKAGE_ROOT}"
    WORKING_DIRECTORY "${OUTPUT_DIRECTORY}"
    RESULT_VARIABLE failed
)

if (failed)
    message(FATAL_ERROR "freeze_paraview failed (${failed})")
endif()

# Move all *.c files to *.h files.
file(GLOB c_files "${OUTPUT_DIRECTORY}/M_*.c")
set (include_headers)
foreach (c_file IN LISTS c_files)
    get_filename_component(h_file "${c_file}" NAME_WE)
    set (h_file "${h_file}.h")
    set (include_headers "${include_headers}#include \"${h_file}\"\n")
    configure_file("${c_file}" "${OUTPUT_DIRECTORY}/${h_file}" COPYONLY)
    file(REMOVE "${c_file}")
endforeach()

# Convert frozen.c to a header file.
file(READ "${OUTPUT_DIRECTORY}/frozen.c" frozen_c_txt)
string(REGEX REPLACE "extern unsigned char (M[a-zA-Z0-9_]+)\\[\\];"
                     "#include \"\\1.h\""
                     frozen_h_txt "${frozen_c_txt}")
string(REGEX REPLACE "main\\([^)]+\\).*{.*}"
               "${OUTPUT_HEADER_PREFIX}()
{
  PyImport_FrozenModules = _PyImport_FrozenModules;
  Py_FrozenFlag = 1;
  Py_NoSiteFlag = 1;
  return 1;
}"
               frozen_h_txt
               "${frozen_h_txt}")
file(WRITE "${OUTPUT_DIRECTORY}/${OUTPUT_HEADER_PREFIX}.h"
           "${frozen_h_txt}")
