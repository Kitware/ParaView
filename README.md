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


Installation
============

ParaView is available as [BioConda](https://bioconda.github.io) package or [Docker/rkt container](https://quay.io/repository/biocontainers/paraview).

You can install it via:

```bash
conda install paraview -c bioconda -c conda-forge
```
or run the container with
```bash
docker run -i -t quay.io/biocontainers/paraview:5.2.0--py27_0 pvpython --version
```


Building
========

The easiest method for beginners to build ParaView from source is using the
[ParaView Superbuild][sbrepo].  It is possible to build ParaView without using the
superbuild as well. A little dated version of those instructions are
available [here][buildinstall].  These will be updated in near future
(contributions are welcome).

[sbrepo]: https://gitlab.kitware.com/paraview/paraview-superbuild
[buildinstall]: http://www.paraview.org/Wiki/ParaView:Build_And_Install

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
