# More work required for MacOS. See #22889
if (APPLE)
  return()
endif()

if (NOT TARGET ParaView::paraview)
  return()
endif()

# build directory is one candidate for site settings files.
set (SiteSettingsDir "${CMAKE_BINARY_DIR}")
file (MAKE_DIRECTORY ${SiteSettingsDir})
# But Qt expects .ini files to be under an ${organization_name} subdirectory
file (MAKE_DIRECTORY ${SiteSettingsDir}/${organization_name})

# ParaView testing suite adds the "--dr" flag that modify settings
# filename with "-dr" extension. Mimic that for testing purpose.
set(suffix "-dr")

# Custom configuration file that makes "advanced" properties visible by default
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/${app_name}.ini.in"
  "${SiteSettingsDir}/${organization_name}/${app_name}${suffix}.ini" @ONLY)

# Custom configuration file that overrides some SphereSource property visibilities.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/PropertyPanelVisibilities.json.in"
  "${SiteSettingsDir}/PropertyPanelVisibilities.json" @ONLY)

# Test to check visibility of the different elements, accordingly to both config files.
set (PropertyPanelVisibilitiesOverride_USES_DIRECT_DATA ON)
paraview_add_client_tests(
  CLIENT "$<TARGET_FILE:${app_name}>"
  TEST_SCRIPTS PropertyPanelVisibilitiesOverride.xml
)
