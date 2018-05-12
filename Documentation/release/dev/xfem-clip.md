# MooseXfemClip Plugin

The MooseXfemClip filter has been added as a plugin to aid
in the visualization of results produced using the XFEM module
in the MOOSE framework (http://www.mooseframework.org)
developed at Idaho National Laboratory.

The MOOSE XFEM implementation uses a phantom node approach
to represent discontinuities in finite element solutions,
which results in overlapping finite elements. The MooseXfemClip
filter clips off the non-physical portions of the overlapping
elements, allowing correct visualization of the discontinuous
fields produced by this method.
