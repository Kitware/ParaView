![ParaView](Documentation/img/paraview100.png)

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

[ParaView]: http://www.paraview.org
[VTK]: http://www.vtk.org
[Kitware]: http://www.kitware.com
[Sandia]: http://www.sandia.gov/
[LANL]: http://www.lanl.gov/
[ARL]: http://www.arl.army.mil/

Learning Resources
==================

* General information is available at the [ParaView Homepage][].

* [The ParaView Guide][Guide] can be downloaded (as PDF) or purchased (in print).

* Community discussion takes place on the [ParaView Mailing Lists][].

* Commercial [support][Kitware Support] and [training][Kitware Training]
  are available from [Kitware][].

* Additional documentation, including Doxygen-generated nightly
  reference documentation, is available [online][Documentation].

[ParaView Homepage]: http://www.paraview.org
[Documentation]: http://www.paraview.org/documentation/
[ParaView Mailing Lists]: http://www.paraview.org/mailing-lists/
[Kitware]: http://www.kitware.com/
[Kitware Support]: http://www.kitware.com/products/support.html
[Kitware Training]: http://www.kitware.com/products/protraining.php
[Guide]: http://www.paraview.org/paraview-guide/


Building
========

There are two ways to build ParaView:

* Perhaps the easiest method for beginners to build ParaView from source is
using the [ParaView Superbuild][sbrepo]. The superbuild downloads and builds all
of ParaView's dependencies as well as ParaView itself.

* It is also possible to [build ParaView][build] without using the superbuild.
ParaView's dependencies must be available on the system.

[sbrepo]: https://gitlab.kitware.com/paraview/paraview-superbuild
[build]: Documentation/dev/build.md

Reporting Bugs
==============

If you have found a bug:

1. If you have a source-code fix, please read the [CONTRIBUTING.md][] document.

2. Otherwise, please join the one of the [ParaView Mailing Lists][] and ask
   about the expected and observed behaviors to determine if it is
   really a bug.

3. Finally, if the issue is not resolved by the above steps, open
   an entry in the [ParaView Issue Tracker][].

[ParaView Issue Tracker]: https://gitlab.kitware.com/paraview/paraview/issues

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
[ParaView Licenses]: http://www.paraview.org/paraview-license/
