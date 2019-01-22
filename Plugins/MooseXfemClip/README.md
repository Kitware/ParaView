XFEM Paraview Plugin
=====

### Description
MooseXfemClip is a filter for use visualizing results produced by
the extended finite element method (XFEM) using the xfem module in the
[MOOSE framework](http://www.mooseframework.org).

The MOOSE XFEM implementation uses the phantom node technique, in which elements
traversed by a discontinuity are split into two partial elements, each containing
physical and non-physical material.

This code generates two sets of elemental variables: `xfem_cut_origin_[0-2]` and
`xfem_cut_normal_[0-2]`, which define the origin and normal of a cutting plane to be
applied to each partial element. If these both contain nonzero values, this filter will
cut off the non-physical portions of those elements.

It is necessary to define the cut planes in this way rather than using a global signed
distance function because a global signed distance function has ambiguities at crack tips,
where two partial elements share a common edge or face.

### Usage Instructions
To use the module, load in all variables coming from an XFEM results file.  The ones
the plugin needs are called: `xfem_cut_origin_[xyz]` and `xfem_cut_normal_[xyz]`.
These are generated automatically by MOOSE models using XFEM.  Once you've done that,
you can apply the MooseXfemClip filter, and you should see the nonphysical parts of
partial elements clipped.

### Acknowledgements
This filter is based on the vtkClipDataSet filter, copyright Ken Martin, Will Schroeder,
and Bill Lorensen.

### Other Software
Idaho National Laboratory is a cutting edge research facility which is a constantly producing high quality research and software. Feel free to take a look at our other software and scientific offerings at:

[Primary Technology Offerings Page](https://www.inl.gov/inl-initiatives/technology-deployment)

[Supported Open Source Software](https://github.com/idaholab)

[Raw Experiment Open Source Software](https://github.com/IdahoLabResearch)

[Unsupported Open Source Software](https://github.com/IdahoLabCuttingBoard)

### License
Copyright 2017 Battelle Energy Alliance, LLC
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of the <organization> nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE ENERGY ALLIANCE, LLC BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### Author contact information
1. Benjamin Spencer
   benjamin.spencer@inl.gov
   Idaho National Laboratory
   P.O. Box 1625, MS 3840
   Idaho Falls, ID 83415
