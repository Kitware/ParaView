## Rework Ray Tracing and ANARI integration

### Improve Ray Tracing and ANARI support in UI

Checkboxes named **Enable Ray Tracing** and **Enable Rendering with ANARI** are now shown only if ParaView is shipped with it.

`pqOpenVRHidingDecorator.h` has also been removed as the class is never used and its `.cxx` file does not exist.

### Rework Ray Tracing and ANARI CMake logic

`PARAVIEW_ENABLE_ANARI` is now independent from `PARAVIEW_ENABLE_RAYTRACING`. Therefore, `PARAVIEW_ENABLE_ANARI` can now be turned `ON` to run ParaView only with ANARI rendering capabilities.
