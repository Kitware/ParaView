# Build Sphinx documentation for Python modules.
if (NOT PARAVIEW_USE_PYTHON)
  return()
endif()

include(ParaViewFindPythonModules)
find_python_module("sphinx" sphinx_found)
if (NOT sphinx_found)
  message(WARNING
    "Please install Sphinx to generate documentation for Python modules.")
  return ()
endif ()

# Copy files referenced from docstrings.
configure_file(
  "${ParaView_SOURCE_DIR}/Examples/Plugins/PythonAlgorithm/Plugin/PythonAlgorithmExamples.py"
  "${CMAKE_CURRENT_BINARY_DIR}/PythonAlgorithmExamples.py"
  @ONLY)

set(sphinx_config_files
  config/_themes/sphinx_rtd_theme/static/css/badge_only.css
  config/_themes/sphinx_rtd_theme/static/css/fonts/Roboto-Slab-Bold.woff
  config/_themes/sphinx_rtd_theme/static/css/fonts/Roboto-Slab-Bold.woff2
  config/_themes/sphinx_rtd_theme/static/css/fonts/Roboto-Slab-Regular.woff
  config/_themes/sphinx_rtd_theme/static/css/fonts/Roboto-Slab-Regular.woff2
  config/_themes/sphinx_rtd_theme/static/css/fonts/fontawesome-webfont.eot
  config/_themes/sphinx_rtd_theme/static/css/fonts/fontawesome-webfont.svg
  config/_themes/sphinx_rtd_theme/static/css/fonts/fontawesome-webfont.tff
  config/_themes/sphinx_rtd_theme/static/css/fonts/fontawesome-webfont.ttf
  config/_themes/sphinx_rtd_theme/static/css/fonts/fontawesome-webfont.woff
  config/_themes/sphinx_rtd_theme/static/css/fonts/fontawesome-webfont.woff2
  config/_themes/sphinx_rtd_theme/static/css/fonts/lato-bold-italic.woff
  config/_themes/sphinx_rtd_theme/static/css/fonts/lato-bold-italic.woff2
  config/_themes/sphinx_rtd_theme/static/css/fonts/lato-bold.woff
  config/_themes/sphinx_rtd_theme/static/css/fonts/lato-bold.woff2
  config/_themes/sphinx_rtd_theme/static/css/fonts/lato-normal-italic.woff
  config/_themes/sphinx_rtd_theme/static/css/fonts/lato-normal-italic.woff2
  config/_themes/sphinx_rtd_theme/static/css/fonts/lato-normal.woff
  config/_themes/sphinx_rtd_theme/static/css/fonts/lato-normal.woff2
  config/_themes/sphinx_rtd_theme/static/css/theme.css
  config/_themes/sphinx_rtd_theme/static/js/badge_only.js
  config/_themes/sphinx_rtd_theme/static/js/theme.js
  config/_themes/sphinx_rtd_theme/__init__.py
  config/_themes/sphinx_rtd_theme/breadcrumbs.html
  config/_themes/sphinx_rtd_theme/footer.html
  config/_themes/sphinx_rtd_theme/locale
  config/_themes/sphinx_rtd_theme/locale/de
  config/_themes/sphinx_rtd_theme/locale/de/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/de/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/en/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/en/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/es/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/es/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/et/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/et/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/fa_IR/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/fa_IR/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/fr/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/fr/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/it/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/it/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/lt/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/lt/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/nl/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/nl/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/pl/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/pl/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/pt/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/pt/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/pt_BR/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/pt_BR/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/ru/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/ru/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/sphinx.pot
  config/_themes/sphinx_rtd_theme/locale/sv/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/sv/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/tr/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/tr/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/locale/zh_CN/LC_MESSAGES/sphinx.mo
  config/_themes/sphinx_rtd_theme/locale/zh_CN/LC_MESSAGES/sphinx.po
  config/_themes/sphinx_rtd_theme/search.html
  config/_themes/sphinx_rtd_theme/searchbox.html
  config/_themes/sphinx_rtd_theme/theme.conf
  config/_themes/sphinx_rtd_theme/versions.html
  config/_themes/sphinx_rtd_theme/layout.html
  config/quick-start.rst
  config/api-changes.rst
  config/conf.py.in
  config/index.rst)

set(_sphinx_depends_args)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
  list(APPEND _sphinx_depends_args
    DEPENDS_EXPLICIT_ONLY)
endif ()

set(paraview_paths
  "['${ParaView_BINARY_DIR}/${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}',
    '${CMAKE_RUNTIME_OUTPUT_DIRECTORY}',
    '${CMAKE_LIBRARY_OUTPUT_DIRECTORY}']")

set(sphinx_copied_files)
foreach (sphinx_config_file IN LISTS sphinx_config_files)
  set(output_config_file
    "${CMAKE_CURRENT_BINARY_DIR}/${sphinx_config_file}")
  if (sphinx_config_file MATCHES "\\.in$")
    string(REPLACE ".in" "" output_config_file "${output_config_file}")
    configure_file(
      "${sphinx_config_file}"
      "${output_config_file}"
      @ONLY)
  else ()
    add_custom_command(
      OUTPUT  "${output_config_file}"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${sphinx_config_file}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              "${CMAKE_CURRENT_SOURCE_DIR}/${sphinx_config_file}"
              "${output_config_file}"
      COMMENT "Copying ${sphinx_config_file} to the binary directory"
      ${_sphinx_depends_args})
  endif ()
  list(APPEND sphinx_copied_files
    "${output_config_file}")
endforeach ()

add_custom_target(paraview_copy_sphinx_config
  DEPENDS
    ${sphinx_copied_files})

set(nompi_flags)
if (PARAVIEW_USE_MPI)
  set(nompi_flags "--no-mpi")
endif ()

set(venv_flags)
if (DEFINED ENV{VIRTUAL_ENV})
  list(APPEND venv_flags
    --venv "$ENV{VIRTUAL_ENV}")
endif ()

add_custom_target(ParaViewPythonDoc
  DEPENDS paraview_copy_sphinx_config
          paraview_all_python_modules
  COMMAND "$<TARGET_FILE:pvpython>"
          ${venv_flags}
          ${nompi_flags}
          "${CMAKE_CURRENT_SOURCE_DIR}/sphinx_apidoc.py"
          -o "${CMAKE_CURRENT_BINARY_DIR}/config"
          "${ParaView_BINARY_DIR}/${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}/paraview"
          # CI utilities end up living here.
          ".gitlab"
          # CI venv
          "venv"
  COMMAND "$<TARGET_FILE:pvpython>"
          ${venv_flags}
          ${nompi_flags}
          -m sphinx
          -b html "${CMAKE_CURRENT_BINARY_DIR}/config"
          "${PARAVIEW_GENERATED_DOCUMENTATION_OUTPUT_DIRECTORY}/python"
  SOURCES sphinx_apidoc.py
          parse_pydoc_output.py
          ${sphinx_config_files}
  COMMENT "Generate Python HTML documentation"
  # ensures that the output is continuously generated
  # when using ninja; avoids builder from timing out
  USES_TERMINAL)
