## New `Point Data Weighing Strategy` option in the `Clean to Grid` filter

When using the `Clean to Grid` filter, you can now choose which strategy to use to collapse your point data.

Previously, when merging duplicate points, the point with the lowest index had its data transported to the merged output point. With this new option, you can now choose between:
* `Take First Point` (for backwards compatibility): where the point with the lowest index in the input gets the ownership of the merged point
* `Average by Number`: where the data on the merged output point is the number average of the input points
* `Average by Spatial Density`: where the merged point data is averaged using a partition of the volumes in the cells attached to each point being merged
