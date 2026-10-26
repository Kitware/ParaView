# New ProbeTable filter

The new **Probe Table** filter samples data from a source `Table` onto any input data.

Unlike most filters, this does not use the geometrical 3D space.
Instead, the **Coordinates Array** selected from the Source can be seen as the axes of a N-dimension space.
Another column is the data of interest, that will be probed.
Each element in the Input can be seen as a random point in this space.
Thus the array names used as coordinates should be found in both the Source table and in the Input data.

Then this space is probed for each element of the Input using linear interpolation.
