## Add a dedicated filter to produce Axis-Aligned slices.

The `Axis-Aligned Slice` filter is now available in ParaView.

This filter produces axis-aligned slices of the input data. The output data type is
the same as the input, effectively reducing the dimensionality of the input data by one.

For now, this filter supports `HyperTree Grids` and `Overlapping AMRs` as input data,
and `Axis-Aligned plane` as cut function. In the case of `HyperTree Grids`, it also
supports producing multiple slices at once.

This filter also supports composite datasets of `HyperTree Grids`. In such cases,
it will iterate over each `HyperTree Grid` and generate a new hierarchy with added
nodes containing the slices.

Please note that usages of `Axis-Aligned plane` in the `Slice` and `Slice With Plane`
filters are deprecated in favor of this new dedicated filter.
