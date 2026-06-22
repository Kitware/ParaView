set (python_plugins
  PythonFilters.py
  PythonReaders.py
  PythonSources.py
  PythonWriters.py
)

if (PARAVIEW_USE_PYTHON)
  paraview_add_plugin(
    PythonAlgorithm
    REQUIRED_ON_SERVER
    VERSION "1.0"
    PYTHON_MODULES ${python_plugins}
  )

  option(BUILD_TESTING "Build Plugin Testing" OFF)
  if (BUILD_TESTING AND BUILD_SHARED_LIBS)
    add_subdirectory(Testing)
  endif()
endif()
