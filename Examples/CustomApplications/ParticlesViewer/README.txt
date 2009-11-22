This example demonstrates how to create specialized applications using the
ParaView's application development framework.

In this example, we are creating a specialized visualization application that
can be used to look at the point-sets. It's basically a stripped down version of
ParaView that allows reading a small sub-set of datasets (*.vtk, *.vtp, *.pvd)
and then displays the points using the point-sprite plugin (distributed with
ParaView).

This example demonstrates the following:
* Building user-interface using a plugin (alternative is to write a new
  QMainWindow subclass).
  - Uses a small set of GUI components used by ParaView (pipeline browser,
    object inspector, and axes controls toolbar) all defined in the UI file
    itself.
  - Specifying set of supported readers.
* Loading required plugins when application startups.
* Changing properties of representations created by default using
  pqDisplayPolicy.

