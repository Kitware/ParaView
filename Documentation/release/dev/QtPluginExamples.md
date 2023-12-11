## Example of plugin using Qt on server side, without PV client

Plugins can make use of Qt classes and resources without
using ParaView Qt modules. This can be the case for instance
in a module using Qt core features.
Then, the module should be built even if ParaView was not built with Qt support.
This is typically the case on server side, where the graphical
interface is not enabled.

A new plugin example named ServerSideQt now illustrates how to do that.
