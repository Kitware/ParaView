from vtk import vtkCornerAnnotation
from vtk import vtkImageData
from vtk import vtkImageViewer2
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWCornerAnnotationEditor
from kwwidgets import vtkKWFrame
from kwwidgets import vtkKWRenderWidget
from kwwidgets import vtkKWWindow
from vtk import vtkRenderWindow
from vtk import vtkRenderWindowInteractor
from vtk import vtkXMLImageDataReader

import os



def vtkKWCornerAnnotationEditorEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a render widget
    # Set the corner annotation visibility
    
    cae_renderwidget = vtkKWRenderWidget()
    cae_renderwidget.SetParent(parent)
    cae_renderwidget.Create()
    cae_renderwidget.CornerAnnotationVisibilityOn()
    
    app.Script("pack %s -side right -fill both -expand y -padx 0 -pady 0",
        cae_renderwidget.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create a volume reader
    
    cae_reader = vtkXMLImageDataReader()
    cae_reader.SetFileName(os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "..", "..", "..", "..", "Data", "head100x100x47.vti"))
    
    # Create an image viewer
    # Use the render window and renderer of the renderwidget
    
    cae_viewer = vtkImageViewer2()
    cae_viewer.SetRenderWindow(cae_renderwidget.GetRenderWindow())
    cae_viewer.SetRenderer(cae_renderwidget.GetRenderer())
    cae_viewer.SetInput(cae_reader.GetOutput())
    cae_viewer.SetupInteractor(
        cae_renderwidget.GetRenderWindow().GetInteractor())
    
    # Reset the window/level and the camera
    
    cae_reader.Update()
    range = cae_reader.GetOutput().GetScalarRange()
    cae_viewer.SetColorWindow(range[1] - range[0])
    cae_viewer.SetColorLevel(0.5 * (range[1] + range[0]))
    
    cae_renderwidget.ResetCamera()
    
    # -----------------------------------------------------------------------
    
    # The corner annotation has the ability to parse "tags" and fill
    # them with information gathered from other objects.
    # For example, let's display the slice and window/level in one corner
    # by connecting the corner annotation to our image actor and
    # image mapper
    
    ca = cae_renderwidget.GetCornerAnnotation()
    ca.SetImageActor(cae_viewer.GetImageActor())
    ca.SetWindowLevel(cae_viewer.GetWindowLevel())
    ca.SetText(2, "<slice>")
    ca.SetText(3, "<window>\n<level>")
    ca.SetText(1, "Hello, World!")
    
    # -----------------------------------------------------------------------
    
    # Create a corner annotation editor
    # Connect it to the render widget
    
    cae_anno_editor = vtkKWCornerAnnotationEditor()
    cae_anno_editor.SetParent(parent)
    cae_anno_editor.Create()
    cae_anno_editor.SetRenderWidget(cae_renderwidget)
    
    app.Script("pack %s -side left -anchor nw -expand n -padx 2 -pady 2",
        cae_anno_editor.GetWidgetName())
    
    return "TypeVTK"
