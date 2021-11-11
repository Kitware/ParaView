## Freeform text in python traces

ParaView client applications may need to trace actions that don't fit
the ParaView pattern of simple function calls or proxy methods. Other
patterns can be supported using the `TraceText` item with the `SM_SCOPED_TRACE`
macro. For example:

```
SM_SCOPED_TRACE(TraceText)
  .arg("op = opMgr.createOperation('smtk::session::mesh::Read')\nresult=op.operate()")
  .arg("comment", "Setup SMTK operation and parameters");

```
