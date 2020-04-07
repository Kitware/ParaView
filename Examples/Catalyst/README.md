# Catalyst Examples Contents
This repository gives examples of how to use ParaView Catalyst (www.paraview.org/in-situ)
for in situ analysis and visualization. For assistance, please visit the Discourse page
at discourse.paraview.org.

The examples show how to create VTK data structures from native simulation code data structures,
how to interface to ParaView Catalyst and how to set up several different types of pipelines.

The tests are very simple and just verify that the examples run, they do not verify
correctness of outputs. Many of the examples work in parallel as well.

# License
Please refer to `LICENSE.md` for the license of this software.

# Build and run
Examples are built as a single external project.
First, user has to build ParaView with Catalyst enabled (`PARAVIEW_USE_CATALYST`).
Most examples require ParaView to have been built with Python (`PARAVIEW_ENABLE_PYTHON`),
except the few with hardcoded C/C++ pipelines. MPI can also be enabled for full
featured examples (`PARAVIEW_ENABLE_MPI`).

See the `CMakeLists.txt` beside this file for Examples related options.

Mainly, examples can be run with the following:
```sh
$ ./<ExampleBinary> <sampleScript.py>
```

for instance :
```sh
$ ./CxxFullExample/CxxFullExample .../CxxFullExample/SampleScripts/feslicescript.py
```

# Description
## Examples
#### FortranPoissonSolver
An example of a parallel, finite difference discretization of the Poisson equation
implemented in Fortran using a Conjugate Gradient solver. Instead of co-processing
at the end of each time step it co-processes at the end of each iteration.
#### Fortran90FullExample
An example of a simulation code written in Fortran
that is linked with Catalyst.
#### CFullExample
An example of a simulation code written in C. This uses some
methods from Catalyst for storing VTK data structures. This
assumes a vtkUnstructuredGrid.
#### CFullExample2
An example of a simulation code written in C. This improves
upon the CFullExample by explicitly storing VTK data structures.
This assumes a vtkUnstructuredGrid.
#### CxxFullExample
A C++ example of a simulation code interfacing with Catalyst.
This assumes a vtkUnstructuredGrid.
#### PythonFullExample
An example of a simulation code written in Python
that uses Catalyst.
#### PythonDolfinExample
An example that uses the Dolfin simulation code.
#### CxxImageDataExample
A C++ example of a simulation code interfacing with
Catalyst. The grid is a vtkImageData.
#### CxxMultiPieceExample
A C++ example of a simulation code interfacing with
Catalyst. The grid is a vtkMultiPiece data set with
a single vtkImageData for each process.
#### CxxNonOverlappingAMRExample
A C++ example of a simulation code interfacing with
Catalyst. The grid is a vtkNonOverlappingAMR.h
data set.
#### CxxOverlappingAMRExample
A C++ example of a simulation code interfacing with
Catalyst. The grid is a vtkOverlappingAMR.h
data set.
#### CxxPVSMPipelineExample
An example where we manually create a Catalyst
pipeline in C++ code using ParaView's server-manager.
This example can be run without ParaView being built
with Python.
#### CxxVTKPipelineExample
An example where we manually create a Catalyst
pipeline in C++ code using VTK filters.
This example can be run without ParaView being built
with Python.
#### CxxMappedDataArrayExample
An example of an adaptor where we use VTK mapped
arrays to map simulation data structures to
VTK data arrays to save on memory use by Catalyst.
Note that this example is deprecated as of
ParaView 5.1 in favor of the CxxSOADataArrayExample.
#### MPISubCommunicatorExample
An example where only a subset of the MPI
processes are used for the simulation and Catalyst.
#### CxxParticlePathExample
An example for computing particle paths in situ.
#### CxxSOADataArrayExample
An example of an adaptor where we use
vtkSOADataArrayTemplate to reuse simulation memory
for VTK field arrays.
#### CxxMultiChannelInputExample
An example that has two adaptor channels/inputs.
The first is an unstructured volumetric grid
with the "volumetric grid" identifier and the
second is a particle grid with the "particles"
identifier. It is tested with the allinputsgridwriter.py
Catalyst Python pipeline script available under the
SampleScripts subdirectory.
#### CxxGhostCellsExample
An example where Catalyst is passed in ghost levels.
This example is meant to be used as an example of how
Catalyst deals with ghost cells for an unstructured grid
or an image data. This example requires a Catalyst build.
#### CxxHyperTreeGridExample
An example where a vtkHyperTreeGrid is produced.

## SamplesScripts
A directory with some useful Python scripts.

### Grid writer
Catalyst scripts `gridwriter.py` and `allinputsgridwriter.py`.

Write out the full dataset/s at a given output frequency specified by the outputfrequency
variable in each script. This can be useful for creating other Catalyst Python pipelines in the ParaView GUI:
write some timesteps on disk to obtain representative data, load them in ParaView, set up a pipeline,
export the corresponding script.

* `gridwriter.py` assumes that there is a single adaptor channel/input that uses the "input" identifier.
* `allinputsgridwriter.py` will write out all adaptor channels/inputs.

Both of these scripts will automatically set the filename based on the channels/inputs identifier string.
For example, for `gridwriter.py` which assumes the "input" identifier string the generated datasets will be called
`input_<time step>.<file extension>`.
The file extension is automatically specified based on the dataset type (e.g. polydata, unstructured grid, etc.).

### LiveVisualization
LiveVisualization is off most of the time. You can manually modify the script to turn it on.
In order to have a quick glance at a simulation, you can use the `livevisu.py` script. It take only
one input and add no filters.

* `livevisu.py` assumes that there is a single adaptor channel/input that uses the "input" identifier.

### File Driver
The script `filedriver.py` can be used as a replacement to a simulation code. It will read a
series of files instead of producing data. It can be run in parallel.
See inner documentation for more.
