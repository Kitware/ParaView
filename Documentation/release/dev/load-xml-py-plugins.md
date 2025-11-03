## Command line to load XML and Python plugins

The `paraview` `--plugins` command line argument to specify plugin names,
can now be used to load XML and Python plugins.
For each searched directory, the lookup procedure looks in order for a library (`.dll` or `.so`),
a Python (`.py`) file and finally a XML (`.xml`). The first match is used.
