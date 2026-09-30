## Fides writer: Select the ADIOS2 engine

The Catalyst **Fides** extractor now provides an **Engine** property to choose
between writing a BP file (**BPFile**, the default) and streaming the data with
the ADIOS2 **SST** engine, and exposes **Adios Config File** to tune the engine.
A Catalyst pipeline can therefore stream data in transit to another process
that opens the same stream:

```python
fides = CreateExtractor("Fides", producer)
fides.Writer.FileName = "data.bp"
fides.Writer.Engine = "SST"
```

Use a file name without timestep substitution to write every timestep to the
same stream. The **Fides Writer** has the same **Engine** property for use from
Python. It is not shown when saving data from the GUI, because an SST writer
waits until a reader connects.
