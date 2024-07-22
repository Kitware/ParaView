# DocsUploader

Automated publication of ParaView documentation.

## selector.js

This script injects a version selector into the C++ and Python documentation.
It selects a header element of the docs and appends a `<select>` element with a
version list as well as a link to the other language documentation.

To build with webpack:
* `npm install`
* `npm run build:release`

The resulting script is `Dist/paraview-version.js`.

## Shell script to automate publication

```
./Scripts/prepare-doc.sh /path/to/paraview/src /path/to/paraview/build /path/to/workdir
```

For nightly or latest (i.e. latest release) you can pass in an additional
parameter on  the command line which is the name to use instead of `git
describe` e.g.

```
./Scripts/prepare-doc.sh /path/to/paraview/src /path/to/paraview/build /path/to/workdir nightly
```

## Updating ParaView docs

Here are steps to update the ParaView docs manually. The instructions are have
been tested on a Linux system.

* Checkout ParaView source for appropriate version.
* Build ParaView with `PARAVIEW_BUILD_DEVELOPER_DOCUMENTATION` CMake flag
  turned ON. You may also want to enable all appropriate features e.g. Python
  support, MPI support. With Python, make sure the `sphinx-build:FILEPATH`
  points to the sphix-build script for correct version of Python.
* Build ParaView normally. This is necessary to ensure everything is built
  correctly.
* Build the `ParaViewDoc-TGZ` target e.g. `ninja ParaViewDoc-TGZ`. This will
  generate the Doxygen and Sphinx generated docs.
* Now run `prepare-doc` script. Provide the optional `version` command line
  argument when generated docs for `latest` or `nightly` instead of using the
  value obtained from `git describe` executed on the source directory. The
  script will update and push the documentation changes to the kitware webserver.
