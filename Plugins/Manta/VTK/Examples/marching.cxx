#include <vtkFloatArray.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkIdList.h>
#include <vtkUnstructuredGrid.h>
#include <vtkContourFilter.h>
#include <vtkExtractEdges.h>
#include <vtkTubeFilter.h>
#include <vtkShrinkPolyData.h>
#include <vtkCubeSource.h>
#include <vtkSphereSource.h>
#include <vtkThresholdPoints.h>
#include <vtkGlyph3D.h>
#include <vtkVectorText.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkMantaRenderer.h"

// This example demonstrates the use of the vtkTransformPolyDataFilter
// to reposition a 3D text string.
double lamp_black[]  = {0.1800, 0.2800, 0.2300};
double banana[]  = {0.8900, 0.8100, 0.3400};
double khaki[]  = {0.9412, 0.9020, 0.5490};
double tomato[]  = {1.0000, 0.3882, 0.2784};
double slate_grey[] = {0.4392, 0.5020, 0.5647};

void case12(vtkFloatArray *scalars, vtkVectorText *caselabel, int IN, int OUT)
{
    scalars->InsertValue(0, OUT);
    scalars->InsertValue(1, IN);
    scalars->InsertValue(2, OUT);
    scalars->InsertValue(3, IN);
    scalars->InsertValue(4, IN);
    scalars->InsertValue(5, IN);
    scalars->InsertValue(6, OUT);
    scalars->InsertValue(7, OUT);
    if (IN == 1)
        caselabel->SetText("Case 12 - 00111010");
    else
        caselabel->SetText("Case 12 - 11000101") ;
}

int main()
{

    // Define a Single Cube
    vtkFloatArray *scalars = vtkFloatArray::New();
    scalars->InsertNextValue(1.0);
    scalars->InsertNextValue(0.0);
    scalars->InsertNextValue(0.0);
    scalars->InsertNextValue(1.0);
    scalars->InsertNextValue(0.0);
    scalars->InsertNextValue(0.0);
    scalars->InsertNextValue(0.0);
    scalars->InsertNextValue(0.0);

    vtkPoints *points = vtkPoints::New();
    points->InsertNextPoint(0, 0, 0);
    points->InsertNextPoint(1, 0, 0);
    points->InsertNextPoint(1, 1, 0);
    points->InsertNextPoint(0, 1, 0);
    points->InsertNextPoint(0, 0, 1);
    points->InsertNextPoint(1, 0, 1);
    points->InsertNextPoint(1, 1, 1);
    points->InsertNextPoint(0, 1, 1);

    vtkIdList *ids = vtkIdList::New();
    ids->InsertNextId(0);
    ids->InsertNextId(1);
    ids->InsertNextId(2);
    ids->InsertNextId(3);
    ids->InsertNextId(4);
    ids->InsertNextId(5);
    ids->InsertNextId(6);
    ids->InsertNextId(7);

    vtkUnstructuredGrid *grid = vtkUnstructuredGrid::New();
    grid->Allocate(10, 10);
    grid->InsertNextCell(12, ids);
    grid->SetPoints(points);
    grid->GetPointData()->SetScalars(scalars);

    vtkContourFilter *marching = vtkContourFilter::New();
    marching->SetInput(grid);
    marching->SetValue(0, 0.5);
    marching->Update();

    // Extract the edges of the triangles just found.
    vtkExtractEdges *triangleEdges = vtkExtractEdges::New();
    triangleEdges->SetInputConnection(marching->GetOutputPort());
    vtkPolyDataMapper *triangleEdgeMapper = vtkPolyDataMapper::New();
    triangleEdgeMapper->SetInputConnection( triangleEdges->GetOutputPort() );
    triangleEdgeMapper->ScalarVisibilityOff();
    vtkActor *triangleEdgeActor = vtkActor::New();
    triangleEdgeActor->SetMapper(triangleEdgeMapper);
    triangleEdgeActor->GetProperty()->SetDiffuseColor(lamp_black);
    triangleEdgeActor->GetProperty()->SetSpecular(.4);
    triangleEdgeActor->GetProperty()->SetSpecularPower(10);
    triangleEdgeActor->GetProperty()->SetLineWidth(4.0);
    triangleEdgeActor->GetProperty()->SetPointSize(16.0);

    // Shrink the triangles we found earlier.  Create the associated mapper
    // and actor.  Set the opacity of the shrunken triangles.
    vtkShrinkPolyData *aShrinker = vtkShrinkPolyData::New();
    aShrinker->SetShrinkFactor(1);
    aShrinker->SetInputConnection(marching->GetOutputPort());
    vtkPolyDataMapper *aMapper = vtkPolyDataMapper::New();
    aMapper->ScalarVisibilityOff();
    aMapper->SetInputConnection(aShrinker->GetOutputPort());
    vtkActor *Triangles = vtkActor::New();
    Triangles->SetMapper(aMapper);
    Triangles->GetProperty()->SetDiffuseColor(banana);
    Triangles->GetProperty()->SetOpacity(.4);

    // Draw a cube the same size and at the same position as the one
    // created previously.  Extract the edges because we only want to see
    // the outline of the cube.  Pass the edges through a vtkTubeFilter so
    // they are displayed as tubes rather than lines.
    vtkCubeSource *cubeModel = vtkCubeSource::New();
    cubeModel->SetCenter(.5, .5, .5);
    vtkExtractEdges *cubeEdges = vtkExtractEdges::New();
    cubeEdges->SetInputConnection(cubeModel->GetOutputPort());
    // Create the mapper and actor to display the cube edges.
    vtkPolyDataMapper *cubeEdgeMapper = vtkPolyDataMapper::New();
    cubeEdgeMapper->SetInputConnection( cubeEdges->GetOutputPort() );
    vtkActor *cubeEdgeActor = vtkActor::New();
    cubeEdgeActor->SetMapper(cubeEdgeMapper);
    cubeEdgeActor->GetProperty()->SetDiffuseColor(khaki);
    cubeEdgeActor->GetProperty()->SetSpecular(.4);
    cubeEdgeActor->GetProperty()->SetSpecularPower(10);
    cubeEdgeActor->GetProperty()->SetLineWidth(4.0);
    
    // Remove the part of the cube with data values below 0.5.
    vtkThresholdPoints *ThresholdIn = vtkThresholdPoints::New();
    ThresholdIn->SetInput(grid);
    ThresholdIn->ThresholdByUpper(.5);

    // Create a mapper and actor to display the glyphs.
    vtkPolyDataMapper *SphereMapper = vtkPolyDataMapper::New();
    SphereMapper->SetInputConnection( ThresholdIn->GetOutputPort() );

    SphereMapper->ScalarVisibilityOff();
    vtkActor *CubeVertices = vtkActor::New();
    CubeVertices->SetMapper(SphereMapper);
    CubeVertices->GetProperty()->SetDiffuseColor(tomato);
    CubeVertices->GetProperty()->SetPointSize(16.0);

    // Define the text for the label
    vtkVectorText *caseLabel = vtkVectorText::New();
    caseLabel->SetText("Case 1");

    // Set up a transform to move the label to a new position.
    vtkTransform *aLabelTransform = vtkTransform::New();
    aLabelTransform->Identity();
    aLabelTransform->Translate(-0.2, 0, 1.25);
    aLabelTransform->Scale(.05, .05, .05);

    // Move the label to a new position.
    vtkTransformPolyDataFilter *labelTransform = vtkTransformPolyDataFilter::New();
    labelTransform->SetTransform(aLabelTransform);
    labelTransform->SetInputConnection(caseLabel->GetOutputPort());

    // Create a mapper and actor to display the text.
    vtkPolyDataMapper *labelMapper = vtkPolyDataMapper::New();
    labelMapper->SetInputConnection(labelTransform->GetOutputPort());

    vtkActor *labelActor = vtkActor::New();
    labelActor->SetMapper(labelMapper);

    // Define the base that the cube sits on.  Create its associated mapper
    // and actor.  Set the position of the actor.
    vtkCubeSource *baseModel = vtkCubeSource::New();
    baseModel->SetXLength(1.5);
    baseModel->SetYLength(.01);
    baseModel->SetZLength(1.5);
    vtkPolyDataMapper *baseMapper = vtkPolyDataMapper::New();
    baseMapper->SetInputConnection(baseModel->GetOutputPort());
    vtkActor *baseActor = vtkActor::New();
    baseActor->SetMapper(baseMapper);
    baseActor->SetPosition(.5, -0.09, .5);

    // Create the Renderer, RenderWindow, and RenderWindowInteractor
    vtkRenderer *renderer = vtkRenderer::New();
    vtkCamera *camera = vtkCamera::New();
    renderer->SetActiveCamera(camera);

    vtkRenderWindow *renderWindow = vtkRenderWindow::New();
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(641, 480);
    vtkRenderWindowInteractor *winInteractor = vtkRenderWindowInteractor::New();
    winInteractor->SetRenderWindow(renderWindow);

    // Add the actors to the renderer
    renderer->AddActor(triangleEdgeActor);
    renderer->AddActor(baseActor);
    renderer->AddActor(labelActor);
    renderer->AddActor(cubeEdgeActor);
    renderer->AddActor(CubeVertices);
    renderer->AddActor(Triangles);

    // Set the background color.
    renderer->SetBackground(slate_grey);

    // Set the scalar values for this case of marching cubes.
    case12(scalars, caseLabel, 0, 1);

    // Force the grid to update.
    grid->Modified();

    // Position the camera.
    renderer->ResetCamera();
    renderer->GetActiveCamera()->Dolly(1.2);
    renderer->GetActiveCamera()->Azimuth(30);
    renderer->GetActiveCamera()->Elevation(20);
    renderer->ResetCameraClippingRange();

    // add a light so we can see shadow
    vtkLight *light = vtkLight::New();
    //light->PositionalOn();

    // Scene light
    //light->SetPosition(renderer->GetActiveCamera()->GetPosition());
    //light->SetFocalPoint(renderer->GetActiveCamera()->GetFocalPoint());

    // Head light
    //light->SetLightTypeToHeadlight();

    // Camera light
    light->SetLightTypeToCameraLight();

    renderer->SetLightFollowCamera(1);
    renderer->AddLight(light);

    winInteractor->Initialize();
    renderWindow->Render();
    winInteractor->Start();

    // clean up
    scalars->Delete();
    points->Delete();
    ids->Delete();
    grid->Delete();
    marching->Delete();
    triangleEdges->Delete();
    triangleEdgeMapper->Delete();
    triangleEdgeActor->Delete();
    aShrinker->Delete();
    aMapper->Delete();
    Triangles->Delete();
    cubeModel->Delete();
    cubeEdges->Delete();
    cubeEdgeMapper->Delete();
    cubeEdgeActor->Delete();
    ThresholdIn->Delete();
    SphereMapper->Delete();
    CubeVertices->Delete();
    caseLabel->Delete();
    aLabelTransform->Delete();
    labelTransform->Delete();
    labelMapper->Delete();
    labelActor->Delete();
    baseModel->Delete();
    baseMapper->Delete();
    baseActor->Delete();
    light->Delete();
    camera->Delete();
    renderer->Delete();
    renderWindow->Delete();
    winInteractor->Delete();
}
