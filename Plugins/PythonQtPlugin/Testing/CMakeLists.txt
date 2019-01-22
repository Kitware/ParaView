find_package("Python${PARAVIEW_PYTHON_VERSION}" QUIET REQUIRED COMPONENTS Interpreter)

set(decorator_file "${CMAKE_CURRENT_BINARY_DIR}/pqPluginDecorators.h")

add_custom_command(
  OUTPUT ${decorator_file}
  COMMAND "Python${PARAVIEW_PYTHON_VERSION}::Interpreter"
          "${CMAKE_CURRENT_SOURCE_DIR}/WrapPythonQt.py"
          -i "${CMAKE_CURRENT_SOURCE_DIR}/wrapped_methods.txt"
          -o "${decorator_file}"
          --class-prefixes vtk pq
          --qt-class-prefixes pq
          --module-name paraview
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wrapped_methods.txt"
          "${CMAKE_CURRENT_SOURCE_DIR}/WrapPythonQt.py")
