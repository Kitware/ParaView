# Adding support for specification of backend when using VTK-m w/ Kokkos

This change propogates build flags from ParaView to VTK and affects how accelerated filters are built
using `VTK-m` It does away with the requirement of using flags like `PARAVIEW_USE_HIP` -- when actually `VTK-m`
is built with the necessary Kokkos backend. This also future-proofs the specification for backends.
Two new flags are introduced `PARAVIEW_USE_KOKKOS` and `PARAVIEW_KOKKOS_BACKEND` -- the first flag is considered
`ON` automatically if the backend flag is provided.

Here's how users can build with different backends

- `PARAVIEW_USE_CUDA` -- use the native CUDA support from VTK-m
- `PARAVIEW_USE_KOKKOS + PARAVIEW_KOKKOS_BACKEND=CUDA` or `PARAVIEW_KOKKOS_BACKEND=CUDA` will use Kokkos with CUDA backend
- `PARAVIEW_USE_KOKKOS + PARAVIEW_KOKKOS_BACKEND=SYCL` or `PARAVIEW_KOKKOS_BACKEND=SYCL` will use Kokkos with SYCL backend
- `PARAVIEW_USE_KOKKOS + PARAVIEW_KOKKOS_BACKEND=HIP` or `PARAVIEW_KOKKOS_BACKEND=HIP` will use Kokkos with HIP backend
- `PARAVIEW_USE_KOKKOS` will use Kokkos with Serial backend
