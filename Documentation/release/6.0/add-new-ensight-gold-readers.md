## Add new EnSight Gold readers

This exposes the new `EnSight Gold Combined Reader` and `EnSight SOS Gold Reader` from VTK. These were rewritten to be easier to maintain, find/fix bugs than the old reader, and it also includes some additional features that the old reader didn't.

- You can select which parts to load, similarly to selecting which arrays to load.
- Static geometry is cached
- Outputs `Partitioned DataSet Collection`
