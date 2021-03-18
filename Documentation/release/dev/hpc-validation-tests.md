## HPC benchmarks and validation suite

To make it easier to test and validate HPC builds, we have added a new package
under the `paraview` Python package called `tests`. This package includes
several modules that test and validate different aspects of the ParaView build.

The tests can be run as follows:


    # all tests
    pvpython -m paraview.tests -o /tmp/resultsdir

    # specific tests
    pvpython -m paraview.tests.verify_eyedomelighting -o /tmp/eyedome.png
    pvpython -m paraview.tests.basic_rendering -o /tmp/basic.png


Use the `--help` or `-h` command line argument for either the `paraview.tests`
package or individual test module to get list of additional options available.

The list is expected to grow over multiple releases. Suggestions to expand this validation
test suite are welcome.
