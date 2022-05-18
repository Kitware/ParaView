## Move Conduit module to VTK and make Catalyst an external dependency

The `VTKExtensions/Conduit` module is now part of VTK, as `IO/CatalystConduit` module.
The `vtkDataObjectToConduit` converter is integrated into this module instead of
its prior `Clients/InSitu` location.

As a side effect, `Catalyst` (our `Conduit` provider) is no more a ThirdParty of ParaView but an external dependency of VTK.

The CMake option `PARAVIEW_ENABLE_CATALYST` control the build of the VTK module and
of the ParaView Catalyst implementation (default OFF)
