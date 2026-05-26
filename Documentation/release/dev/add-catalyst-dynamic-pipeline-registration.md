## Catalyst: dynamic pipeline registration via state/pipelines

ParaView Catalyst now supports registering analysis pipelines at execute
time, in addition to the existing init-time `catalyst/scripts`
registration. This allows adaptors to add pipelines after
`catalyst_initialize` without re-initializing the framework.

At `catalyst_execute` time, each child of `catalyst/state/pipelines` may be
either:

- a string, naming an already-registered pipeline to execute (existing
  behavior), or
- an object `{name, filename, [args]}`, registering the pipeline
  on-the-fly if not already known, then executing it.

Example execute params (YAML presentation of the Conduit node):

```yaml
catalyst:
  state:
    timestep: 100
    time: 0.5
    pipelines:
      - "existing_pipe"           # already registered, select to run
      - name: "slice1"            # new dynamic registration
        filename: "slice.py"
        args:
          - "--x_cut=0.25"
      - name: "slice2"            # another instance of slice.py
        filename: "slice.py"
        args:
          - "--x_cut=0.75"
```

Args set at registration time are routed per-instance via `get_args()` in
the script, so multiple parameterized instances of the same script each
receive their own args without cross-contamination.

Name is identity: an object-form entry referencing an already-registered
name is treated as selection only. If its filename or args differ from
the existing registration, a `WARNING` is logged and the existing
registration is retained. To use different values, register under a new
name. Args are not mutable across executes; use `state/parameters` or
steerable parameters for runtime updates.

The object form dispatches on the `type` field (default `script`):

- `type: "script"` (or absent): Python script pipeline, schema
  `{name, filename, [args]}` where `filename` is the .py script path.
  Requires Python support; if not enabled, the entry is ignored with a
  warning.
- `type: "io"`: C++ pre-compiled file-writer pipeline, schema
  `{name, type, filename, channel}` where `filename` is the output path
  and `channel` selects which mesh to write. The actual
  writer is chosen by extension via `vtkSMWriterFactory` (XML VTK
  formats, legacy VTK, CGNS, Exodus, IOSS, image formats, etc.).
  No Python dependency.
- Unsupported types are rejected by the blueprint verifier.

This mirrors the init-time split between `catalyst/scripts` (Python) and
`catalyst/pipelines` (C++ pre-compiled), unified under a single dynamic
registration path.

A dict-form entry registers and executes the pipeline in the same
`catalyst_execute` call. There is currently no way to register without
also executing this turn; if that is needed, register on the first
execute call where the pipeline should run.

This is a ParaView-side behavior extension to the Catalyst Conduit
protocol. The Catalyst C API is unchanged.
