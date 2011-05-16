# This example demonstrates the python wrapping of vtkManta classes.
# In order to run this example, first be sure that the python wrapping
# of VTK is working, either using vtkpython or the vanilla python interpreter.
# Then you must set the 'PYTHONPATH' environment variable to where the
# library files (libvtkMantaPythonD.so, libvtkMantaPython.so, and
# libvtkManta.so) are located.
from vtk import *
from libvtkMantaPython import *
cone = vtkConeSource()
cone.SetHeight( 3.0 )
cone.SetRadius( 1.0 )
cone.SetResolution( 10 )
coneMapper = vtkMantaPolyDataMapper()
coneMapper.SetInputConnection(cone.GetOutputPort())
coneActor = vtkMantaActor()
coneActor.SetMapper(coneMapper)
ren1 = vtkMantaRenderer()
ren1.AddActor(coneActor)
ren1.SetBackground(0.1, 0.2, 0.4)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(300, 300)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
style = vtkInteractorStyleTrackballCamera()
iren.SetInteractorStyle(style)
renWin.Render()
iren.Initialize()
iren.Start()
