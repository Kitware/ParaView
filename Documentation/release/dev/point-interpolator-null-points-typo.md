## Point Interpolator Null Points Strategy Typo Fix

The PointInterpolatorBase source proxy had a typo that prevented users from
selecting "Closest Point" as the Null Points strategy for the Point Line Interpolator,
Point Plane Interpolator, Point Volume Interpolator, and Point Dataset Interpolator
Filters. Note that this only affects those interpolation kernels that can produce
such null points, such as the Linear Kernel while using a radius based interpolation.
