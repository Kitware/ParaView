// First include the required header files for the VTK classes we are using.
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

double lamp_black[]     = {0.1800, 0.2800, 0.2300};

int main()
{
    //
    // Next we create an instance of vtkConeSource and set some of its
    // properties. The instance of vtkConeSource "cone" is part of a
    // visualization pipeline (it is a source process object); it produces data
    // (output type is vtkPolyData) which other filters may process.
    //
    vtkConeSource *cone = vtkConeSource::New();
    cone->SetHeight(3.0);
    cone->SetRadius(1.0);
    cone->SetResolution(10);

    //
    // In this example we terminate the pipeline with a mapper process object.
    // (Intermediate filters such as vtkShrinkPolyData could be inserted in
    // between the source and the mapper.)  We create an instance of
    // vtkPolyDataMapper to map the polygonal data into graphics primitives. We
    // connect the output of the cone souece to the input of this mapper.
    //
    vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
    coneMapper->SetInputConnection(cone->GetOutputPort());

    //
    // Create an actor to represent the cone. The actor orchestrates rendering
    // of the mapper's graphics primitives. An actor also refers to properties
    // via a vtkProperty instance, and includes an internal transformation
    // matrix. We set this actor's mapper to be coneMapper which we created
    // above.
    //
    vtkActor *coneActor = vtkActor::New();
    coneActor->SetMapper(coneMapper);

    vtkProperty *coneProperty = coneActor->GetProperty();
    coneProperty->SetColor(1.0, 0.3882, 0.2784);
    coneProperty->SetDiffuse(0.7);
    coneProperty->SetSpecular(0.4);
    coneProperty->SetSpecularPower(20);
    coneActor->SetProperty(coneProperty);

    //
    // Create the Renderer and assign actors to it. A renderer is like a
    // viewport. It is part or all of a window on the screen and it is
    // responsible for drawing the actors it has.  We also set the background
    // color here.
    //
    vtkRenderer *renderer= vtkRenderer::New();
    renderer->AddActor(coneActor);
    renderer->SetBackground(0.1, 0.2, 0.4);

    vtkCamera *camera = vtkCamera::New();
    renderer->SetActiveCamera(camera);
    renderer->ResetCamera();

    //
    // Finally we create the render window which will show up on the screen.
    // We put our renderer into the render window using AddRenderer. We also
    // set the size to be 300 pixels by 300.
    //
    vtkRenderWindow *renWin = vtkRenderWindow::New();
    //vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
    renWin->SetSize(480, 480);

    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

    renWin->Render();

    iren->Start();

    // clean up
    cone->Delete();
    coneMapper->Delete();
    coneActor->Delete();
    camera->Delete();
    renderer->Delete();
    renWin->Delete();
    iren->Delete();

    return 0;
}
