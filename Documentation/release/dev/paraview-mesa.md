# paraview-mesa

The `--mesa*` flags have been removed from the `paraview` binary. Instead,
there is now the `paraview-mesa` executable which can launch any ParaView
executable with a Mesa environment. For example, instead of the `--mesa-llvm`
flag, `paraview-mesa paraview --backend llvmpipe -- $paraview_args` may be
used.
