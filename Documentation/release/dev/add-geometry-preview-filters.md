## Add geometry preview filters

ParaView now has 2 new filters named `PointSetToOctreeImage` and `OctreeImageToPointSet`.

1) `PointSetToOctreeImage` can be used to convert a point set to an image with a number of points per cell
   target and an unsigned char octree cell array. Each bit of the unsigned char indicates if the point-set had a
   point close to one of the 8 corners of the cell. It can optionally also output a cell array based on an input
   point array. This array will have 1 or many components that represent different functions i.e. last value, min,
   max, count, sum, mean.
2) `OctreeImageToPointSet` can be used to convert an image with an unsigned char octree cell array to a point
   set. Each bit of the unsigned char indicates if the cell had a point close to one of its 8 corners.
   It can optionally also output a point array based on an input cell array. This array will have 1 of the
   components of the input array.
