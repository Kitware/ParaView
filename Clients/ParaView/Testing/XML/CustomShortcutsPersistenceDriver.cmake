# Regression test driver for paraview/paraview#21133.
#
# QSettings(IniFormat, UserScope) reads/writes "$HOME/.config/<Organization>/
# <Application>.ini" on linux/macOS, so a fake HOME pointed at a directory
# pre-populated with INI_FIXTURE lets this test start ParaView as if a
# custom shortcut had already been saved in a previous session, without
# touching real settings.

set(home_dir "${TEMPORARY_DIR}/CustomShortcutsPersistenceHome")
file(REMOVE_RECURSE "${home_dir}")
file(MAKE_DIRECTORY "${home_dir}/.config/ParaView")
file(COPY "${INI_FIXTURE}" DESTINATION "${home_dir}/.config/ParaView")
get_filename_component(ini_fixture_name "${INI_FIXTURE}" NAME)
file(RENAME "${home_dir}/.config/ParaView/${ini_fixture_name}" "${home_dir}/.config/ParaView/ParaView.ini")

set(ENV{HOME} "${home_dir}")
set(ENV{DASHBOARD_TEST_FROM_CTEST} "1")

execute_process(
  COMMAND ${PARAVIEW_EXECUTABLE}
          --test-script=${TEST_SCRIPT}
          --exit
  RESULT_VARIABLE rv
)

file(REMOVE_RECURSE "${home_dir}")

if (NOT rv EQUAL 0)
  message(FATAL_ERROR "ParaView return value was ${rv}")
endif()
