# Launcher arguments for using mesa

On linux, `paraview_mesa` launcher has now been removed. ParaView
binaries now respect command line arguments instead to indicate if Mesa3D
libraries should be loaded for software rendering, instead of using OpenGL
libraries available on your system. Pass `--mesa` with optional `--backend
[swr|llvm]` to any of the ParaView executables to request Mesa3D-based rendering
with approrpiate backend.


# Using MPICH ABI compatible implementation

ParaView binaries can now be used on linux systems with MPICH ABI compatible
MPI implementation. Compatible MPI implementations are listed
[here](https://wiki.mpich.org/mpich/index.php/ABI_Compatibility_Initiative). To
use system MPI implementation instead of the one packaged with ParaView, pass
`--system-mpi` command line argument to any of the ParaView executables.
