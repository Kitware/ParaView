## Fix for CDI Reader 3D mask allocation

The CDI Reader now correctly handles memory allocation for multilayer 3D mask data. Previously, it allocated based on maximum vertical levels rather than the actual number of levels in the dataset, causing out-of-bounds access. This fix ensures proper indexing and prevents memory corruption when reading mask data with varying layer counts.
