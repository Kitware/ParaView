# Add Force Static Mesh

Introduce a new filter, named **Force Static Mesh**, that enables supported
temporal data with a constant geometry to have a constant `GetMeshMTime()`.
This information can be exploited by supsequent filters like the **Surface
Filter** for performance and memory optimizations.
