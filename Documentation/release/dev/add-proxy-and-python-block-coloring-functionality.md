## Add Proxy and Python block coloring functionality

``vtkCompositePolyDataMapper`` has been recently improved to provide block coloring capabilities. ParaView now exposes
these capabilities by adding new block specific properties and functions to the ``vtkSMPVRepresentationProxy`` along
with python functions including `ColorBlockBy` and ``GetBlockColorTransferFunction``. An example can be found
in ``Clients/ParaView/Testing/Python/ColorBlockBy.py`` test.

This work will be the foundation of a refactoring of the ParaView's ``MultiBlock Inspector`` in the near future.
