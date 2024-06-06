## Add Angle and CustomField strategies for Polyline decimation

The Decimate Polyline filter now support multiple decimation strategies :
- `DecimationDistanceStrategy` (default, identical to previous versions): use the distance between consecutive points as error metric;
- `DecimationAngleStrategy` : use the angle between 3 consecutive points as error metric;
- `DecimationCustomFieldStrategy`: use values stored in a custom PointData array to compute the error between 3 consecutive points. The name of this array must be defined using the property panel
