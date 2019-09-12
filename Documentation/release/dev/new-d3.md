# Redistribute dataset filter

Newly added **RedistributeDataSet** filter can be used to split data into the
requested number of partitions for load balancing. The filter either computes
the bounding boxes, or uses a provided collection of bounding boxes, for
distributing the data. Cells along partition boundaries can be uniquely
assigned to a partition, duplicated among partitions, or split among the
partitions.
