# Adding support for XML testing in external plugins

When adding an XML testing to an external plugin, it is now possible
to specify a variable that will be picked up by paraview
testing macro. With the variable defined, no data expansion will be performed
which means that baseline can now be used without requiring to rely
on an ExternalData mechanism and directly point to baseline image
or data in the plugin subdirectory.

See Examples/Plugins/ElevationFilter for an example
