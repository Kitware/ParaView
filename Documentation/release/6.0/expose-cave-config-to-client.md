## Expose CAVE configuration to client

In CAVE mode, pvservers are normally given a pvx file to read
containing the geometry of each display, as well as the physical
coordinates of the screen corners of each display.

This change makes those values available to the client via new
api on the render view proxy.  Clients can query whether or not
the system is in CAVE mode (the rest of the api methods will
complain if not in CAVE mode), as well as query the number of
displays.  For each display the client can ask for the geometry
and screen corners.

Example:

```python
>>> from paraview.simple import *
>>> rv = GetActiveView()
>>> rv.GetIsInCAVE()
True
>>> rv.GetHasCorners(0)
True
>>> rv.GetLowerLeft(0)
vtkmodules.vtkCommonMath.vtkTuple_IdLi3EE([-1.13, -0.635, 1.04])
>>> rv.GetLowerRight(0)
vtkmodules.vtkCommonMath.vtkTuple_IdLi3EE([-1.13, -0.635, -1.22])
>>> rv.GetUpperRight(0)
vtkmodules.vtkCommonMath.vtkTuple_IdLi3EE([-1.13, 0.635, -1.22])
>>> rv.GetGeometry(0)
vtkmodules.vtkCommonMath.vtkTuple_IiLi4EE([60, 1200, 640, 360])

```
