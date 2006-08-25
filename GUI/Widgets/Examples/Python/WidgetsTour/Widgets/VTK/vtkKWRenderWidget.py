from vtk import vtkActor
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWRenderWidget
from kwwidgets import vtkKWWindow
from vtk import vtkPolyDataMapper
from vtk import vtkRenderWindow
from vtk import vtkXMLPolyDataReader

import os



def vtkKWRenderWidgetEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a render widget
    
    rw_renderwidget = vtkKWRenderWidget()
    rw_renderwidget.SetParent(parent)
    rw_renderwidget.Create()
    
    app.Script("pack %s -side top -fill both -expand y -padx 0 -pady 0",
        rw_renderwidget.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a 3D object reader
    
    rw_reader = vtkXMLPolyDataReader()
    rw_reader.SetFileName(os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "..", "..", "..", "..", "Data", "teapot.vtp"))
    
    # Create the mapper and actor
    
    rw_mapper = vtkPolyDataMapper()
    rw_mapper.SetInputConnection(rw_reader.GetOutputPort())
    
    rw_actor = vtkActor()
    rw_actor.SetMapper(rw_mapper)
    
    # Add the actor to the scene
    
    rw_renderwidget.AddViewProp(rw_actor)
    rw_renderwidget.ResetCamera()
    
    
    
    return "TypeVTK"
