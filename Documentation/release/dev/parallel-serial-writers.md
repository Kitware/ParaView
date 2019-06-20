# Improvements to several non-parallel writers (STL, PLY, VTK, and others)

Writers for file formats such as STL, PLY, legacy VTK, which do not support
parallel writing traditionally reduced all data to the root node when running
in parallel, and then wrote the data out on that node alone. To better support
large data use-cases, the user can now choose a subset of ranks to reduce
the data to and write files on. Using contiguous or round-robin groupping on
ranks, this mechanism enables use-cases where multiple MPI ranks are executing on
the same node and the user wants to nominate one rank per node to do the IO.
