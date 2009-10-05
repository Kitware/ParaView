#include "vtkSTLReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkLODActor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkToolkits.h"

#include <string>

// This simple example shows how to do basic rendering and pipeline
// creation. It also demonstrates the use of the LODActor.

static double vtk_light_grey[3] = { 0.8275, 0.8275, 0.8275 };

int main()
{
    // This creates a polygonal cylinder model with eight circumferential
    // facets.
    string filename = VTK_DATA_ROOT;
    filename += "/Data/42400-IDGH.stl";
    vtkSTLReader *part = vtkSTLReader::New();
    part->SetFileName(filename.c_str());

    // The mapper is responsible for pushing the geometry into the graphics
    // library. It may also do color mapping, if scalars or other
    // attributes are defined.
    vtkPolyDataMapper *partMapper = vtkPolyDataMapper::New();
    partMapper->SetInputConnection(part->GetOutputPort());

    // The LOD actor is a special type of actor. It will change appearance
    // in order to render faster. At the highest resolution, it renders
    // ewverything just like an actor. The middle level is a point cloud,
    // and the lowest level is a simple bounding box.
    vtkLODActor *partActor = vtkLODActor::New();
    partActor->SetMapper(partMapper);
    partActor->GetProperty()->SetColor(vtk_light_grey);
    partActor->RotateX(30.0);
    partActor->RotateY(-45.0);

    // Create the graphics structure. The renderer renders into the render
    // window. The render window interactor captures mouse events and will
    // perform appropriate camera or actor manipulation depending on the
    // nature of the events.
    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren);
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

    // Add the actors to the renderer, set the background and size
    ren->AddActor(partActor);
    ren->SetBackground(0.1, 0.2, 0.4);
    renWin->SetSize(600, 600);

    // We'll zoom in a little by accessing the camera and invoking a "Zoom"
    // method on it.
    ren->ResetCamera();
    ren->GetActiveCamera()->Zoom(1.5);

    iren->Initialize();
    renWin->Render();
    iren->Start();

    // delete dyanmically created objects
    part->Delete();
    partMapper->Delete();
    partActor->Delete();
    ren->Delete();
    renWin->Delete();
    iren->Delete();
}
