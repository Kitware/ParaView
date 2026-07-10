## Fides in-memory Catalyst channel and reader improvements

Catalyst pipelines can now exchange data with ParaView through a live in-memory
Conduit tree read by Fides, and the Fides readers expose more of the underlying
data model.

* A new `fides_conduit` Catalyst V2 channel type lets a simulation hand ParaView
  a Fides data model together with a live Conduit node tree, with no ADIOS file
  or SST stream in between. The schema declares a single Conduit data source and
  the dispatcher binds the channel's node to it directly, so the reader reads
  through the simulation's in-memory data each step. Each channel carries its own
  schema and gets an independent **Fides Reader** producer. The channel layout
  mirrors the other Catalyst channel types:

  ```
  catalyst/channels/<name>/type        = "fides_conduit"
  catalyst/channels/<name>/schema_file = "/path/to/model.json"   (or)
  catalyst/channels/<name>/schema      = "<inline JSON string>"
  catalyst/channels/<name>/data        = <conduit mesh node>
  ```

* A new **Fides** extractor (available under `Extractors -> Data`) wraps the
  Fides writer so pipeline scripts can persist data via ADIOS2 BP output with
  `CreateExtractor("Fides", source, ...)`. This is the only extractor that
  understands cell-grid datasets, which the default extractor skips.

* The **Fides Reader** and **Fides Data Model** readers now expose a **CellGrid
  Attributes** array-selection group, so cell-grid attributes can be toggled on
  and off independently of the point, cell, and field array selections.

* The per-step simulation time is now forwarded through the `fides_conduit`
  channel to the reader output, so an in-situ BP written by the Fides extractor
  carries real per-step time values instead of a 0..N-1 index.

Developer notes: `vtkFidesReader` no longer exposes `SetConvertToVTK` (the
Viskores-to-VTK conversion is now unconditional), and the stale `ConvertToVTK`
property has been dropped from both Fides reader proxies.
