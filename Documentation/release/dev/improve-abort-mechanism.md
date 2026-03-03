## Improve abort mechanism

 - ParaView _abort_ button is now only enabled after apply is pressed instead of whenever an progress
event is received, which could lead to instability.
 - ParaView _abort_ button now works in client/server and client/server/render mode

 Please note distributed server abort is currently non-fonctionnal.

### Dev notes
 - The "Abort" feature can be enabled at the source proxy level and is activated for any SMSourceProxy working with a SISourceProxy.
 - vtkPVProgressHandler is responsible of the client/server sync
