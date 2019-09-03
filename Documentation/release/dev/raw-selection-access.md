# Provide access to client-side selection

The vtkSMRenderViewProxy class now creates instances
of a new vtkPVEncodeSelectionForServer class in order
to prepare the client-side selection for representations
on the server side. This allows applications built on
ParaView a way to access the client-side selection
while it is being processed.
