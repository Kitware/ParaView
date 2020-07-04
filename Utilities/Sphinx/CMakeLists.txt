# Build Sphinx documentation for Python modules.
if (NOT PARAVIEW_USE_PYTHON)
  return()
endif()

include(FindPythonModules)
find_python_module("sphinx" sphinx_found)
if (NOT sphinx_found)
  message(STATUS
    "Please install Sphinx to generate documentation for Python modules.")
  return()
endif()

# Copy Sphinx configuration files to the build directory.
file(COPY config
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
     PATTERN "*.in" EXCLUDE)

set (paraview_paths
  "['${ParaView_BINARY_DIR}/${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}',
    '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}',
    '${CMAKE_LIBRARY_OUTPUT_DIRECTORY}']")
configure_file(config/conf.py.in config/conf.py @ONLY)

# This depends on the python modules being placed at the appropriate locations.
# Since all python files are copies to lib/site-packages at configure time and
# since this CMakeLists.txt file is added at the end of the top-level
# CMakeLists.txt file, that indeed is true.
add_custom_target(ParaViewPythonDoc
  COMMAND $<TARGET_FILE:pvpython>
          --no-mpi
          "${CMAKE_CURRENT_SOURCE_DIR}/sphinx_apidoc.py"
          -o "${CMAKE_CURRENT_BINARY_DIR}/config"
          "${ParaView_BINARY_DIR}/${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}/paraview"
  COMMAND $<TARGET_FILE:pvpython>
          --no-mpi
          -m sphinx
          -b html "${CMAKE_CURRENT_BINARY_DIR}/config"
          "${PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY}/python"
  SOURCES sphinx_apidoc.py
          config/index.rst
          config/quick-start.rst
          config/api-changes.rst
          config/conf.py.in
  # ensures that the output is continuously generated
  # when using ninja; avoids builder from timing out
  USES_TERMINAL)
