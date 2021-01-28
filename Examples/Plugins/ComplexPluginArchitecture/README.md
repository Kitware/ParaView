# Complexe module architecture examples plugins

This example shows how to create a complex architecture of VTK modules and ParaView plugins.
It highlights the following:

1. How to create a ParaView plugin containing multiple VTK modules within
2. How to create a VTK module that will be shared by multiple Plugins
3. How to trigger the ClientServer wrapping of this shared module (see ComplexModuleArchitecture/ParaViewPlugin/CMakeLists.txt)
