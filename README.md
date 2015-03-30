![ParaView](Documentation/img/paraview100.png)

Introduction
============
ParaView is an open-source, multi-platform data analysis and
visualization application. ParaView users can quickly build
visualizations to analyze their data using qualitative and
quantitative techniques. The data exploration can be done
interactively in 3D or programmatically using ParaView's batch
processing capabilities.

ParaView was developed to analyze extremely large datasets using
distributed memory computing resources. It can be run on
supercomputers to analyze datasets of petascale size as well as
on laptops for smaller data, has become an integral tool in many
national laboratories, universities and industry, and has won
several awards related to high performance computation.

The ParaView code base is designed in such a way that all of its
components can be reused to quickly develop vertical applications.
This flexibility allows ParaView developers to quickly develop
applications that have specific functionality for a specific problem
domain. Under the hood, ParaView uses the Visualization Toolkit (VTK)
as the data processing and rendering engine and has a user interface
written using Qt.

History
========
The ParaView project started in 2000 as a collaborative effort between
Kitware Inc. and Los Alamos National Laboratory. The initial funding was
provided by a three-year contract with the US Department of Energy ASCI
Views program. The first public release, ParaView 0.6, was announced
in October 2002.

Independent of ParaView, Kitware started developing a web-based visualization
system in December 2001. This project was funded by Phase I and II SBIRs from
the US Army Research Laboratory and eventually became the ParaView Enterprise
Edition. PVEE significantly contributed to the development of ParaView's
client/server architecture.

Since the beginning of the project, Kitware has successfully collaborated with
Sandia, Los Alamos National Laboratories, the Army Research Laboratory and
various other academic and government institutions to continue development.
The project is still going strong!

In September 2005, Kitware, Sandia National Labs and CSimSoft started the
development of ParaView 3.0. This was a major effort focused on rewriting the
user interface to be more user-friendly and on developing a quantitative
analysis framework. ParaView 3.0 was released in May 2007.

Learning Resources
==================

* General information is available at the [ParaView Homepage][].

* Community discussion takes place on the [ParaView Mailing Lists][].

* Commercial [support][Kitware Support] and [training][Kitware Training]
  are available from [Kitware][].

* Additional documentation including Doxygen-generated nightly
  reference documentation is available [online][Documentation].

[ParaView Homepage]: http://www.paraview.org
[Documentation]: http://www.paraview.org/documentation/
[ParaView Mailing Lists]: http://www.paraview.org/mailing-lists/
[Kitware]: http://www.kitware.com/
[Kitware Support]: http://www.kitware.com/products/support.html
[Kitware Training]: http://www.kitware.com/products/protraining.php

Reporting Bugs
==============

If you have found a bug:

1. If you have a patch, please read the [CONTRIBUTING.md][] document.

2. Otherwise, please join the one of the [ParaView Mailing Lists][] and ask
   about the expected and observed behaviors to determine if it is
   really a bug.

3. Finally, if the issue is not resolved by the above steps, open
   an entry in the [ParaView Issue Tracker][].

[ParaView Issue Tracker]: http://www.paraview.org/Bug

Contributing
============

See [CONTRIBUTING.md][] for instructions to contribute.

[CONTRIBUTING.md]: CONTRIBUTING.md

License
=======

ParaView is distributed under the OSI-approved BSD 3-clause License.
See [Copyright.txt][] for details. For additional licenses, refer to
[ParaView Licenses][].

[Copyright.txt]: Copyright.txt
[ParaView Licenses]: http://www.paraview.org/paraview-license/
