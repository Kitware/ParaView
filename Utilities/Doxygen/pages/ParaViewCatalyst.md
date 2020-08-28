ParaView Catalyst {#ParaViewCatalyst}
=================

**This page is applicable to ParaView Catalyst implementation introduced in
ParaView 5.9.**

-----

## Introduction

Prior to ParaView version 5.9, for a simulation code to use ParaView for in situ
processing required developing an adaptor which had two parts: convert
simulation data structures to VTK Data Object, and use classes provided by
ParaView (collectively referred to as Catalyst) to initialize, and then execute
the analysis pipelines on each simulation cycle.

Converting simulation data structures to VTK Data Objects is a non-trivial task
and requires understanding of how VTK stores internal arrays and build data
objects. Simple mistakes could result in invalid memory accesses or costly
data-copies impacting memory requirements and performance adversely.

Setting up and invoking ParaView via the Catalyst classes requires creating and
using classes such as `vtkCPProcessor`, `vtkCPPipeline` and subclasses,
`vtkCPInputDataDescription`, `vtkCPDataDescription`, and several others.

In other words, the adaptor ended up with a lot of ParaView specific C++ code that
potentially changed between each version of ParaView and needed updates to
accommodate new capabilities added.

Since the simulation directly links against the custom Catalyst adaptor
developed specifically for the simulation, the simulation too is tightly
coupled to a specific version of ParaView. Once built, it isn't possible to
switch which version of ParaView is being used without rebuilding the
adaptor and the simulation.

To build an adaptor, you need a ParaView SDK. Since ParaView binaries do not
provide headers and libraries that would comprise an SDK, you have to built
ParaView from source. That itself can be daunting task adding further to the
complexity and learning curve.

To minimize several of these challenges, we decided to revisit the design and
implementation of the various components involved. To avoid confusion, all the
Catalyst and in situ components described so far that are available prior to
ParaView 5.9 are referred to as Legacy.

## Catalyst and ParaView-Catalyst

The new design is built on following key components:

* A stable API that simulation codes can use to describe data and
  invoke in situ processing pipelines.

* A lightweight implementation of this API that can be used to build simulations
  when using this API.

* An implementation of this API that uses ParaView for data processing that is
  ABI compatible with the lightweight implementation and hence can be
  dynamically replaced at load-time when launching the simulation.

The stable API is now called the [Catalyst API]. It is a C-only API that includes
mechanisms to describe data and other control parameters (using [Conduit API])
and trigger in situ processing. It is provided in a [separate project] together
with a lightweight implementation called the **stub**.

The compatible ParaView-specific implementation of the Catalyst API is now called
ParaView-Catalyst and is built and distributed as part of the ParaView
distribution. [ParaView-Catalyst Blueprint](@ref ParaViewCatalystBlueprint) describes
parameters supported by this Catalyst implementation for providing scripts to
load, computational meshes etc.

A typical Catalyst adaptor developed for a specific simulation, in this new
approach, no longer builds VTK data objects, instead simply describes its data
structures using an implementation-supported protocol. ParaView-Catalyst, the
canonical implementation of the Catalyst API that uses ParaView, provides
several ways of describing data and will continue to evolve to include a large
set of data-structures and memory layouts used by codes.

If that's not adequate, developers can develop their own custom implementation
for the Catalyst API. Such implementations are of course free to use whatever
data processing and visualization libraries the developers choose. They can also
use `vtkInSituInitializationHelper`, `vtkInSituPipeline` and subclasses, to
use ParaView as the in situ processing engine.

[Catalyst API]: https://catalyst-in-situ.readthedocs.io/en/latest/?badge=latest
[Conduit API]: https://llnl-conduit.readthedocs.io/en/latest/index.html
[separate project]: https://gitlab.kitware.com/paraview/catalyst
