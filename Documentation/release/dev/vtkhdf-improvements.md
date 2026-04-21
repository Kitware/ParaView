## VTKHDF file Reader: Select field data arrays to read

You can now select which field data arrays to read from the Properties panel.
This used to be possible only for point and cell arrays.

## VTKHDF file Reader: Select piece distribution in distributed

When reading partitioned VTKHDF data in parallel, you can now choose how blocks are allocated to processes.
The "Block" mode will allocate partitions by block to each pvserver process, and "Interleave" will allocate using a round-robin algorithm.
