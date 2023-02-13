![ParaView](Utilities/Doxygen/images/paraview-logo-small.png)

Introduction
============
[ParaView][] is an open-source, multi-platform data analysis and
visualization application based on
[Visualization Toolkit (VTK)][VTK].

The first public release was announced in October 2002. Since then, the project
has grown through collaborative efforts between [Kitware Inc.][Kitware],
[Sandia National Laboratories][Sandia],
[Los Alamos National Laboratory][LANL],
[Army Research Laboratory][ARL], and various other
government and commercial institutions, and academic partners.

[ParaView]: https://www.paraview.org
[VTK]: https://www.vtk.org
[Kitware]: https://www.kitware.com
[Sandia]: https://www.sandia.gov/
[LANL]: https://www.lanl.gov/
[ARL]: https://www.arl.army.mil/

Learning Resources
==================

* General information is available at the [ParaView Homepage][].

* The ParaView User's Guide can be found on the [ParaView Documentation][Guide].

* Community discussion takes place on the [ParaView Discourse][] forum.

* Commercial [support and training][Kitware Support]
  are available from [Kitware][].

* Additional documentation, including Doxygen-generated nightly
  reference documentation, is available [online][Resources].

[ParaView Homepage]: https://www.paraview.org
[Resources]: https://www.paraview.org/resources/
[ParaView Discourse]: https://discourse.paraview.org/
[Kitware]: https://www.kitware.com/
[Kitware Support]: https://www.kitware.com/support/
[Guide]: https://docs.paraview.org/en/latest/


Building
========

There are two ways to build ParaView:

* The easiest method for begginners to build ParaView from source is
by using our [Getting Started compilation guide][build] which includes
commands to install the needed dependencies for most operating systems.

* Another way to build ParaView, quite useful when trying to enable more specific
options which requires to build dependencies yourself (ie osmesa, raytracing),
would be the [ParaView Superbuild][sbrepo]. The superbuild downloads and builds all
of ParaView's dependencies as well as ParaView itself.

[sbrepo]: https://gitlab.kitware.com/paraview/paraview-superbuild
[build]: Documentation/dev/build.md

Reporting Bugs
==============

If you have found a bug:

1. If you have a source-code fix, please read the [CONTRIBUTING.md][] document.

2. Otherwise, please join the [ParaView Discourse][] forum and ask about
   the expected and observed behaviors to determine if it is really a bug.

3. Finally, if the issue is not resolved by the above steps, open
   an entry in the [ParaView Issue Tracker][].

[ParaView Issue Tracker]: https://gitlab.kitware.com/paraview/paraview/-/issues

Contributing
============

See [CONTRIBUTING.md][] for instructions to contribute.

For Github users
----------------

[Github][] is a mirror of the [official repository][repo]. We do not actively monitor issues or
pull requests on Github. Please use the [official repository][repo] to report issues or contribute
fixes.

[Github]: https://github.com/Kitware/ParaView
[repo]: https://gitlab.kitware.com/paraview/paraview
[CONTRIBUTING.md]: CONTRIBUTING.md

License
=======

ParaView is distributed under the OSI-approved BSD 3-clause License.
See [Copyright.txt][] for details. For additional licenses, refer to
[ParaView Licenses][].

[Copyright.txt]: Copyright.txt
[ParaView Licenses]: https://www.paraview.org/license/
