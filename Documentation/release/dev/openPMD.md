## openPMD

Avoid to scale meshes and particle records with `unitSI` equal to `1.0`.
This is often the case for non-floating point records and unintentionally casts their type to double when we mean to stay in integer or unsigned integer (e.g., particle `id`) scales.
This thus fixes issues with the latter and avoid unnecessary operations on large data sets.
