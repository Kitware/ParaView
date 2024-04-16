## Add Angle and CustomField strategies for Polyline decimation

The `vtkDecimatePolylineFilter` has recently been refactored to support different decimation strategies. Paraview now takes advantage of this change and provides access to 2 new Polyline decimation strategies :
- `DecimationAngleStrategy`
- `DecimationCustomFieldStrategy`

The `DecimationAngleStrategy` uses the angle between 3 consecutive points as a metric of error.

The `DecimationCustomFieldStrategy` uses values stored in a custom PointData array to compute the error between 3 consecutive points. The name of this array must be defined using the property panel when using this strategy.
