## ExplodeDataSet can name output parts from FieldData

When its new **NamesFromArray** property is On, the **ExplodeDataSet**
filter will look in the input FieldData for a **PartitionNames** array
containing the names to use for the output partitions.
This array should be ordered as **PartitionValues**, listing the
expected values for the **Scalars** cell array.

If the mapping fails (scalar not present in **PartitionValues** or
index not found in **PartitionNames**) the automatic name is used
(scalar array name suffixed with the partition index)
