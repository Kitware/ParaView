#include "vtkCylinderSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"

double lamp_black[]     = {0.1800, 0.2800, 0.2300};

int main()
{
    //
    // Next we create an instance of vtkCylinderSource and set some of its
    // properties. The instance of vtkCylinderSource "Cylinder" is part of a
    // visualization pipeline (it is a source process object); it produces data
    // (output type is vtkPolyData) which other filters may process.
    //
    vtkCylinderSource *Cylinder = vtkCylinderSource::New();
    Cylinder->SetHeight(3.0);
    Cylinder->SetRadius(1.0);
    Cylinder->SetResolution(5);

    //
    // In this example we terminate the pipeline with a mapper process object.
    // (Intermediate filters such as vtkShrinkPolyData could be inserted in
    // between the source and the mapper.)  We create an instance of
    // vtkPolyDataMapper to map the polygonal data into graphics primitives. We
    // connect the output of the Cylinder souece to the input of this mapper.
    //
    vtkPolyDataMapper *CylinderMapper = vtkPolyDataMapper::New();
    CylinderMapper->SetInputConnection(Cylinder->GetOutputPort());

    //
    // Create an actor to represent the Cylinder. The actor orchestrates rendering
    // of the mapper's graphics primitives. An actor also refers to properties
    // via a vtkProperty instance, and includes an internal transformation
    // matrix. We set this actor's mapper to be CylinderMapper which we created
    // above.
    //
    vtkActor *CylinderActor = vtkActor::New();
    CylinderActor->SetMapper(CylinderMapper);

    vtkProperty *CylinderProperty = CylinderActor->GetProperty();
    CylinderProperty->SetColor(1.0, 0.3882, 0.2784);
    CylinderProperty->SetDiffuse(0.7);
    CylinderProperty->SetSpecular(0.4);
    CylinderProperty->SetSpecularPower(20);
    CylinderProperty->SetInterpolationToFlat();
    CylinderActor->SetProperty(CylinderProperty);

    //
    // Create the Renderer and assign actors to it. A renderer is like a
    // viewport. It is part or all of a window on the screen and it is
    // responsible for drawing the actors it has.  We also set the background
    // color here.
    //
    vtkRenderer *renderer= vtkRenderer::New();
    renderer->AddActor(CylinderActor);
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
    renWin->AddRenderer(static_cast<vtkRenderer*>(renderer));
    renWin->SetSize(480, 480);

    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    //vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

    renWin->Render();

    iren->Start();

    // clean up
    Cylinder->Delete();
    CylinderMapper->Delete();
    CylinderActor->Delete();
    camera->Delete();
    renderer->Delete();
    renWin->Delete();
    iren->Delete();

    return 0;
}
