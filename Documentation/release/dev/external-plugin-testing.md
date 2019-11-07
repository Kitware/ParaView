# Adding support for testing in external plugins

ParaView now support XML and Python testing within external plugins.

When adding a XML testing to an external plugin, it is now possible
to specify a variable, `testName_USES_DIRECT_DATA`, that will be picked up by paraview
testing macro. With the variable defined, no data expansion will be performed
which means that baseline can now be used without requiring to rely
on an ExternalData mechanism and directly point to baseline image
or data in the plugin subdirectory.

```
set (TestName_USES_DIRECT_DATA ON)
paraview_add_client_tests(
  LOAD_PLUGIN PluginName
  PLUGIN_PATH $<TARGET_FILE_DIR:PluginName>
  BASELINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline
  TEST_SCRIPTS TestName.xml)

```

When adding a python testing to an external plugin, it is now possible
to specify a parameter to the paraview_add_test_python function, `DIRECT_DATA`,
that will be picked up by VTK testing macro. With this parameter defined,
no data expansion will be performed which means that baseline can now be used
without requiring to rely on an ExternalData mechanism and directly point to baseline image
or data in the plugin subdirectory.
In order to load the plugin in the python script, it may be required to use
other mechanisms, see in the elevation filter example.

```
# Uses DIRECT_DATA to inform VTK to look into Data/Baseline/ for the baselines
paraview_add_test_python(
  NO_RT DIRECT_DATA
  TestPython.py)
```

See Examples/Plugins/ElevationFilter for a complete example
