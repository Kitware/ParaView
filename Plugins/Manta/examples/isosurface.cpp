// given .vtk files as arguments, loads the meshes and puts them in a scene
// rendered using Manta

#include <stdlib.h>

// include the required header files for the VTK classes we are using.
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPolyDataReader.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLight.h"
#include "vtkRegularPolygonSource.h"
#include "vtkSphereSource.h"

#include "vtkMantaProperty.h"

#include <Engine/Control/RTRT.h>


#define NUM_COLORS 2
//double colors[2][3] = { {0.9412, 0.9020, 0.5490},
double colors[2][3] = { {0.6421, 0.6020, 0.1490},
                        {0.9000, 0.9000, 0.9000}};
                        //{0.5000, 0.2882, 0.1784}};

double checkerColors[2][3] = { {0.9412, 0.2020, 0.1490},
                               {0.7000, 0.7000, 0.7000}};

double banana[] = {0.8900, 0.8100, 0.3400};
double slate_grey[] = {0.4392, 0.5020, 0.5647};
double lamp_black[] = {0.1800, 0.2800, 0.2300};

void addCheckerboard(vtkRenderer* renderer)
{
    double center[3] = {0.5, 0.0, 0.5};
    int numSquares = 20;
    double length = 0.25;
    double start_x = center[0] - length*numSquares / 2.0;
    double start_z = center[2] - length*numSquares / 2.0;
    double orig_z = start_z;
    int i, j;

    for(i=0; i<numSquares; i++)
    {
        for(j=0; j<numSquares; j++)
        {
            vtkRegularPolygonSource* poly = vtkRegularPolygonSource::New();
            poly->SetNumberOfSides(4);
            poly->SetCenter(start_x+length/2.0, 0.3, start_z+length/2.0);
            poly->SetNormal(0, 1, 0);
            poly->SetRadius(0.7071*length);

            vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
            mapper->SetInputConnection(poly->GetOutputPort());
            vtkActor* actor = vtkActor::New();
            actor->SetMapper(mapper);
            actor->SetOrigin(actor->GetCenter());
            actor->RotateY(45);

            actor->GetProperty()->SetOpacity(1.0);
            actor->GetProperty()->SetSpecularPower(100);

            int k = (i+j) % 2;
            actor->GetProperty()->SetDiffuseColor(checkerColors[k][0],
                                                  checkerColors[k][1],
                                                  checkerColors[k][2]);

            renderer->AddActor(actor);

            start_z += length;
        }
        start_x += length;
        start_z = orig_z;
    }
}

void addSphere(vtkRenderer* renderer)
{
    vtkSphereSource* sphere = vtkSphereSource::New();
    sphere->SetRadius(0.2);
    sphere->SetThetaResolution(100);
    sphere->SetPhiResolution(100);

    vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
    mapper->SetInputConnection(sphere->GetOutputPort());
    vtkActor* actor = vtkActor::New();
    actor->SetMapper(mapper);

    vtkMantaProperty* property = vtkMantaProperty::SafeDownCast(
                                                         actor->GetProperty());
    property->SetDiffuseColor(0.9, 0.9, 0.9);
    property->SetOpacity(0.5);
    property->SetMaterialType("thindielectric");

    renderer->AddActor(actor);
}

int main(int argc, char** argv)
{
    int i;

    if(argc < 2)
    {
        printf("usage: isosurface <file1> <file2> ...\n");
        printf("Only .vtk files are supported\n");
        printf("exiting...\n");
        exit(-1);
    }

    // all arguments should be filenames to .vtk files
    int numInputs = argc - 1;

    // get the filenames
    char** filenames = (char**)malloc(sizeof(char*)*numInputs);
    for(i=0; i<numInputs; i++)
    {
        filenames[i] = argv[i+1];
    }

    // load the .vtk files containing geometry information
    vtkPolyDataReader** readers = (vtkPolyDataReader**)malloc(
                                        sizeof(vtkPolyDataReader*)*numInputs);
    for(i=0; i<numInputs; i++)
    {
        readers[i] = vtkPolyDataReader::New();
        readers[i]->SetFileName(filenames[i]);
    }

    vtkPolyDataMapper** dataMappers =
             (vtkPolyDataMapper**)malloc(sizeof(vtkPolyDataMapper*)*numInputs);
    for(i=0; i<numInputs; i++)
    {
        dataMappers[i] = vtkPolyDataMapper::New();
        dataMappers[i]->SetInputConnection(readers[i]->GetOutputPort());
        dataMappers[i]->ScalarVisibilityOff();
    }

    // create actors for each file
    vtkActor** actors=(vtkActor**)malloc(sizeof(vtkActor*)*numInputs);
    for(i=0; i<numInputs; i++)
    {
        actors[i] = vtkActor::New();
        actors[i]->SetMapper(dataMappers[i]);

        // the index of the color to use
        int j = i % NUM_COLORS;

        vtkMantaProperty* property = vtkMantaProperty::SafeDownCast(
                                                     actors[i]->GetProperty());

        property->SetDiffuseColor(colors[j][0], colors[j][1], colors[j][2]);
        property->SetSpecularColor(0.5, 0.5, 0.5);
        property->SetSpecular(0.2);
        property->SetSpecularPower(100);
        //property->SetOpacity(0.9);
        property->SetMaterialType("phong");
        //property->SetMaterialType("thindielectric");
        property->SetEta(2.52);
        property->SetThickness(0.4);
    }

    // create the renderer
    vtkRenderer *renderer = vtkRenderer::New();
    for(i=0; i<numInputs; i++)
    {
        renderer->AddActor(actors[i]);
    }

    // add checkerboard
    //addCheckerboard(renderer);
    //addSphere(renderer);

    renderer->SetBackground(slate_grey);

    vtkCamera *cam = vtkCamera::New();
    renderer->SetActiveCamera(cam);
    renderer->ResetCamera();

    // create lights
    vtkLight *light1 = vtkLight::New();
    light1->PositionalOn();
    light1->SetPosition(renderer->GetActiveCamera()->GetPosition());
    light1->SetFocalPoint(renderer->GetActiveCamera()->GetFocalPoint());
    light1->SetColor(0.6, 0.6, 0.6);
    light1->SetLightTypeToCameraLight();
    renderer->SetLightFollowCamera(1);
    renderer->AddLight(light1);

    vtkLight *light2 = vtkLight::New();
    light2->PositionalOn();
    light2->SetPosition(1, 1, 0);
    light2->SetFocalPoint(0, 0, 0);
    light2->SetColor(0.6, 0.6, 0.6);
    light2->SetLightTypeToCameraLight();
    renderer->SetLightFollowCamera(1);
    renderer->AddLight(light2);

    // create the render window
    vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(static_cast<vtkRenderer*>(renderer));
    renWin->SetSize(400, 400);

    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

    // set it to trackball style
    vtkInteractorStyleTrackballCamera *style =
                                vtkInteractorStyleTrackballCamera::New();
    iren->SetInteractorStyle(style);

    renWin->Render();
    iren->Start();

    // cleanup
    for(i=0; i<numInputs; i++)
    {
        readers[i]->Delete();
        dataMappers[i]->Delete();
        actors[i]->Delete();
    }

    free(filenames);
    free(readers);
    free(dataMappers);
    free(actors);

    renderer->Delete();
    renWin->Delete();
    cam->Delete();
    light1->Delete();
    light2->Delete();
    iren->Delete();
    style->Delete();

    return 0;
}
