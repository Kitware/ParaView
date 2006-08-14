from vtk import vtkImageViewer2
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWHeaderAnnotationEditor
from kwwidgets import vtkKWFrame
from kwwidgets import vtkKWRenderWidget
from kwwidgets import vtkKWWindow
from vtk import vtkRenderWindow
from vtk import vtkRenderWindowInteractor
from vtk import vtkXMLImageDataReader

import os



def vtkKWHeaderAnnotationEditorEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a render widget
    # Set the header annotation visibility and set some text
    
    hae_renderwidget = vtkKWRenderWidget()
    hae_renderwidget.SetParent(parent)
    hae_renderwidget.Create()
    
    hae_renderwidget.HeaderAnnotationVisibilityOn()
    hae_renderwidget.SetHeaderAnnotationText("Hello, World!")
    
    app.Script("pack %s -side right -fill both -expand y -padx 0 -pady 0",
        hae_renderwidget.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a volume reader
    
    hae_reader = vtkXMLImageDataReader()
    hae_reader.SetFileName(os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "..", "..", "..", "Data", "head100x100x47.vti"))
    
    # Create an image viewer
    # Use the render window and renderer of the renderwidget
    
    hae_viewer = vtkImageViewer2()
    hae_viewer.SetRenderWindow(hae_renderwidget.GetRenderWindow())
    hae_viewer.SetRenderer(hae_renderwidget.GetRenderer())
    hae_viewer.SetInput(hae_reader.GetOutput())
    hae_viewer.SetupInteractor(
        hae_renderwidget.GetRenderWindow().GetInteractor())
    
    hae_renderwidget.ResetCamera()
    
    # -----------------------------------------------------------------------
    
    # Create a header annotation editor
    # Connect it to the render widget
    
    hae_anno_editor = vtkKWHeaderAnnotationEditor()
    hae_anno_editor.SetParent(parent)
    hae_anno_editor.Create()
    hae_anno_editor.SetRenderWidget(hae_renderwidget)
    
    app.Script("pack %s -side left -anchor nw -expand n -padx 2 -pady 2",
        hae_anno_editor.GetWidgetName())
    
    return "TypeVTK"
