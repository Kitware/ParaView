## Global Point And Cell Ids numbers all the partitions together

The **Global Point And Cell Ids** filter now numbers the points and cells of all the partitions of a Multi-block
Dataset, a Partitioned Dataset or a Partitioned Dataset Collection together. Previously, each partition was numbered on
its own, so the same id was given to unrelated points of different partitions, and points that should not be merged
ended up sharing an id.
