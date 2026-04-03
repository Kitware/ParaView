## AggregateDataSet: Add TargetProcess property

The **AggregateDataSet** filter now has a new property called **TargetProcess**. This property allows you to specify
the process on which the dataset will be aggregated, The options are:

1. _Root Process_: data from all processes is aggregated to the root process of the (sub)controller. This ensures
   consistent aggregation across timesteps even when point counts change over time.
2. _Process With Most Points_ (default): data from all processes is aggregated to the process that currently has the
   most points, minimizing the amount of data sent over the network. Note that if the process with the most points
   changes over time, the aggregation target may vary across timesteps.
