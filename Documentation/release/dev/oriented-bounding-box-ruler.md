# Bounding Ruler can now measure axes along oriented bounding boxes

In addition to measuring the X, Y, or Z axis of the axis-aligned bounding box
of a data set, the **Bounding Ruler** filter can now measure the Major, Medium,
and Minor axes of the data set's oriented bounding box. These axes are new options
in the **Axis** property of the **Bounding Ruler** filter. As an example, one
can use ParaView's selection capabilities to extract two points of interest and
then apply the **Bounding Ruler** with the **Axis** property set to
**Oriented Bounding Box Major Axis** to measure how the distance between these
points changes in time.

One caveat to these options is that all the points in a data set need to be copied
to a single rank to compute the oriented bounding box. When used in a parallel
server setting, this may be slow or lead to memory exhaustion if the data set does
not fit onto one rank.

|![Measuring the length of a rotated cylinder object along its oriented bounding box major axis](img/5.6.0/OrientedBoundingBoxMajorAxis.png)|
|:--:|
| *Measuring the length of a rotated cylinder object along its oriented bounding box major axis.* |
