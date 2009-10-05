// load a .vtk file into a scene, and output it to an image file
// arguments are <input> <output>

// include the required header files for the VTK classes we are using.
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLight.h"
#include "vtkPoints.h"
#include "vtkTransformFilter.h"
#include "vtkTransform.h"
#include "vtkCellArray.h"
#include "vtkParametricRandomHills.h"
#include "vtkParametricFunctionSource.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkCleanPolyData.h"
#include "vtkPolyDataReader.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"
#include "vtkCommand.h"
#include "vtkActorCollection.h"
#include "vtkProperty.h"
#include "vtkConeSource.h"
#include "vtkStripper.h"

#include <Engine/Display/SyncDisplay.h>
#include <Engine/Control/RTRT.h>

#include <stdlib.h>
#include <ostream>

// macros
#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif

#define USE_MANTA
#define SAVE_IMAGE true
#define SHOW_IMAGE true
#define AUTO_CAMERA true
#define LOAD_FILE true

#ifdef USE_MANTA
    #include "vtkMantaProperty.h"
    #include "vtkMantaRenderer.h"
#endif

void addBox(vtkRenderer* renderer);
void addCone(vtkRenderer* renderer);
void addBox2(vtkRenderer* renderer);
void addBoundingBox(vtkRenderer* renderer);
vtkActor* addFile(vtkRenderer* renderer, const char* filename);
void addLights(vtkRenderer* renderer);
void saveImage(vtkRenderer* renderer, const char* filename);

double slate_grey[] = {0.4392, 0.5020, 0.5647};

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        printf("Error! arguments: <input> <output>\n");
        return -1;
    }

    int i;

    // create renderer
    vtkRenderer *renderer = vtkRenderer::New();
    renderer->SetBackground(slate_grey);

    // add the objects in the scene
    addBox(renderer);
    if(LOAD_FILE)
    {
        addFile(renderer, argv[1]);
    }
    else
    {
        addBox2(renderer);
        //addCone(renderer);
    }
    addLights(renderer);
    //addBoundingBox(renderer);

    renderer->ResetCamera();
    vtkCamera* cam = renderer->GetActiveCamera();
    if(AUTO_CAMERA)
    {
        cam->Dolly(1.2);
        cam->Azimuth(65);
        cam->Elevation(25);
        cam->Zoom(1.7);
        cam->Print(cout);
    }
    else
    {
        cam->SetClippingRange(4.38454, 8.55403);
        cam->SetFocalPoint(0, -0.00390601, -0.00390601);
        cam->SetPosition(4.26015, 2.188, 1.98263);
        cam->SetViewAngle(17.6471);
        cam->ComputeViewPlaneNormal();
        cam->SetViewUp(0, 1, 0);
    }

    vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(static_cast<vtkRenderer*>(renderer));
    renWin->SetSize(400, 400);

    if(!SHOW_IMAGE)
    {
        renWin->OffScreenRenderingOn();
    }

    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

    // set interaction it to trackball style
    vtkInteractorStyleTrackballCamera *style =
                                vtkInteractorStyleTrackballCamera::New();
    iren->SetInteractorStyle(style);

    if(SAVE_IMAGE)
    {
        renWin->Render();
        sleep(1);
        saveImage(renderer, argv[2]);
    }
    else
    {
        iren->Start();
    }

    // clean up
    renderer->Delete();
    renWin->Delete();
    iren->Delete();

    printf("done!\n");
    return 0;
}

void saveImage(vtkRenderer* renderer, const char* filename)
{
    // save the screen output to a png file
    vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
    w2i->SetInput(renderer->GetRenderWindow());

    vtkPNGWriter* writer = vtkPNGWriter::New();
    writer->SetInput(w2i->GetOutput());
    writer->SetFileName(filename);
    writer->Write();
    printf("image saved to %s\n", filename);

    w2i->Delete();
    writer->Delete();
}

void addBox(vtkRenderer* renderer)
{
    // add certain faces of a bounding box to the scene
    const char* material = "phong";
    double box_color[] = {0.5, 0.2, 0.2};

    double xmin = -1.0;
    double xmax = 1.0;
    double ymin = -0.896875;
    double ymax = 0.889063;
    double zmin = -0.896875;
    double zmax = 0.889063;

    float pos[8][3];
    pos[0][0] = xmin; pos[0][1] = ymin; pos[0][2] = zmin;
    pos[1][0] = xmax; pos[1][1] = ymin; pos[1][2] = zmin;
    pos[2][0] = xmax; pos[2][1] = ymax; pos[2][2] = zmin;
    pos[3][0] = xmin; pos[3][1] = ymax; pos[3][2] = zmin;
    pos[4][0] = xmin; pos[4][1] = ymin; pos[4][2] = zmax;
    pos[5][0] = xmax; pos[5][1] = ymin; pos[5][2] = zmax;
    pos[6][0] = xmax; pos[6][1] = ymax; pos[6][2] = zmax;
    pos[7][0] = xmin; pos[7][1] = ymax; pos[7][2] = zmax;

    int numSides = 3;
    static vtkIdType pointIds[][4]= {
                                      {0, 1, 2, 3}, // back
                                      //{4, 5, 6, 7}, // front
                                      {0, 1, 5, 4}, // bottom
                                   //{1, 2, 6, 5}, // right
                                      //{2, 3, 7, 6}, // top
                                      {3, 0, 4, 7}, // left
                                     };

    // We'll create the building blocks of polydata including data attributes.
    vtkPolyData* cube = vtkPolyData::New();
    vtkPoints* points = vtkPoints::New();
    vtkCellArray* polys = vtkCellArray::New();

    // Load the point, cell, and data attributes.
    int i;
    for (i=0; i<8; i++)
        points->InsertPoint(i, pos[i]);
    for (i=0; i<numSides; i++)
        polys->InsertNextCell(4, pointIds[i]);

    // We now assign the pieces to the vtkPolyData.
    cube->SetPoints(points);
    points->Delete();
    cube->SetPolys(polys);
    polys->Delete();

    // Now we'll look at it.
    vtkPolyDataMapper *cubeMapper = vtkPolyDataMapper::New();
    cubeMapper->SetInput(cube);
    vtkActor *cubeActor = vtkActor::New();
    cubeActor->SetMapper(cubeMapper);

    vtkProperty* property = cubeActor->GetProperty();
    property->SetDiffuseColor(box_color);
    property->SetSpecularColor(box_color);

#ifdef USE_MANTA
    vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(
                                                     cubeActor->GetProperty());
    mantaProperty->SetMaterialType(material);
#endif

    renderer->AddActor(cubeActor);
}

void addBox2(vtkRenderer* renderer)
{
    // add certain faces of a bounding box to the scene
    const char* material = "phong";
    double box_color[] = {0.4, 0.5333, 0.1529};

    double xmin =-0.5;
    double xmax = 0.5;
    double ymin =-0.5;
    double ymax = 0.5;
    double zmin =-0.5;
    double zmax = 0.5;

    float pos[8][3];
    pos[0][0] = xmin; pos[0][1] = ymin; pos[0][2] = zmin;
    pos[1][0] = xmax; pos[1][1] = ymin; pos[1][2] = zmin;
    pos[2][0] = xmax; pos[2][1] = ymax; pos[2][2] = zmin;
    pos[3][0] = xmin; pos[3][1] = ymax; pos[3][2] = zmin;
    pos[4][0] = xmin; pos[4][1] = ymin; pos[4][2] = zmax;
    pos[5][0] = xmax; pos[5][1] = ymin; pos[5][2] = zmax;
    pos[6][0] = xmax; pos[6][1] = ymax; pos[6][2] = zmax;
    pos[7][0] = xmin; pos[7][1] = ymax; pos[7][2] = zmax;

    int numSides = 6;
    static vtkIdType pointIds[][4]= {
                                      {0, 1, 2, 3}, // back
                                      {4, 5, 6, 7}, // front
                                      {0, 1, 5, 4}, // bottom
                                   {1, 2, 6, 5}, // right
                                      {2, 3, 7, 6}, // top
                                      {3, 0, 4, 7}, // left
                                     };

    // We'll create the building blocks of polydata including data attributes.
    vtkPolyData* cube = vtkPolyData::New();
    vtkPoints* points = vtkPoints::New();
    vtkCellArray* polys = vtkCellArray::New();

    // Load the point, cell, and data attributes.
    int i;
    for (i=0; i<8; i++)
        points->InsertPoint(i, pos[i]);
    for (i=0; i<numSides; i++)
        polys->InsertNextCell(4, pointIds[i]);

    // We now assign the pieces to the vtkPolyData.
    cube->SetPoints(points);
    points->Delete();
    cube->SetPolys(polys);
    polys->Delete();

    // Now we'll look at it.
    vtkPolyDataMapper *cubeMapper = vtkPolyDataMapper::New();
    cubeMapper->SetInput(cube);
    vtkActor *cubeActor = vtkActor::New();
    cubeActor->SetMapper(cubeMapper);

    vtkProperty* property = cubeActor->GetProperty();
    property->SetDiffuseColor(box_color);
    property->SetSpecularColor(box_color);

#ifdef USE_MANTA
    vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(
                                                     cubeActor->GetProperty());
    mantaProperty->SetMaterialType(material);
#endif

    renderer->AddActor(cubeActor);
}

void addCone(vtkRenderer* renderer)
{
    // add a cone to the scene
    vtkConeSource *cone = vtkConeSource::New();
    cone->SetHeight(1.0);
    cone->SetRadius(0.5);
    cone->SetResolution(50);

    vtkStripper* strip = vtkStripper::New();
    strip->SetInputConnection(cone->GetOutputPort());

    vtkPolyDataMapper *coneMapper = vtkPolyDataMapper::New();
    //coneMapper->SetInputConnection(cone->GetOutputPort());
    coneMapper->SetInputConnection(strip->GetOutputPort());

    vtkActor *coneActor = vtkActor::New();
    coneActor->SetMapper(coneMapper);

    vtkProperty *coneProperty = coneActor->GetProperty();
    coneProperty->SetColor(1.0, 0.3882, 0.2784);
    coneProperty->SetDiffuse(0.7);
    coneProperty->SetSpecular(0.4);
    coneProperty->SetSpecularPower(20);
    coneActor->SetProperty(coneProperty);

#ifdef USE_MANTA
    vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(coneProperty);
    mantaProperty->SetMaterialType("dielectric");
    mantaProperty->SetN(1.0);
    mantaProperty->SetNt(1.3);
#endif

    renderer->AddActor(coneActor);
}

void addBoundingBox(vtkRenderer* renderer)
{
    // add a bounding box to the scene
    const char* material = "lambertian";
    double box_color[] = {0.9, 0.9, 0.9};

    double epsilon = 0.1;
    double xmin =-1.0 - epsilon;
    double xmax = 1.0 + epsilon;
    double ymin =-1.0 - epsilon;
    double ymax = 1.0 + epsilon;
    double zmin =-1.0 - epsilon;
    double zmax = 1.0 + epsilon;

    float pos[8][3];
    pos[0][0] = xmin; pos[0][1] = ymin; pos[0][2] = zmin;
    pos[1][0] = xmax; pos[1][1] = ymin; pos[1][2] = zmin;
    pos[2][0] = xmax; pos[2][1] = ymax; pos[2][2] = zmin;
    pos[3][0] = xmin; pos[3][1] = ymax; pos[3][2] = zmin;
    pos[4][0] = xmin; pos[4][1] = ymin; pos[4][2] = zmax;
    pos[5][0] = xmax; pos[5][1] = ymin; pos[5][2] = zmax;
    pos[6][0] = xmax; pos[6][1] = ymax; pos[6][2] = zmax;
    pos[7][0] = xmin; pos[7][1] = ymax; pos[7][2] = zmax;

    int numSides = 6;
    static vtkIdType pointIds[][4]= {
                                      {0, 1, 2, 3}, // back
                                      {4, 5, 6, 7}, // front
                                      {0, 1, 5, 4}, // bottom
                                   {1, 2, 6, 5}, // right
                                      {2, 3, 7, 6}, // top
                                      {3, 0, 4, 7}, // left
                                     };

    // We'll create the building blocks of polydata including data attributes.
    vtkPolyData* cube = vtkPolyData::New();
    vtkPoints* points = vtkPoints::New();
    vtkCellArray* polys = vtkCellArray::New();

    // Load the point, cell, and data attributes.
    int i;
    for (i=0; i<8; i++)
        points->InsertPoint(i, pos[i]);
    for (i=0; i<numSides; i++)
        polys->InsertNextCell(4, pointIds[i]);

    // We now assign the pieces to the vtkPolyData.
    cube->SetPoints(points);
    points->Delete();
    cube->SetPolys(polys);
    polys->Delete();

    // Now we'll look at it.
    vtkPolyDataMapper *cubeMapper = vtkPolyDataMapper::New();
    cubeMapper->SetInput(cube);
    vtkActor *cubeActor = vtkActor::New();
    cubeActor->SetMapper(cubeMapper);

    vtkProperty* property = cubeActor->GetProperty();
    property->SetDiffuseColor(box_color);
    property->SetSpecularColor(box_color);

#ifdef USE_MANTA
    vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(
                                                     cubeActor->GetProperty());
    mantaProperty->SetMaterialType(material);
#endif


    renderer->AddActor(cubeActor);
}

vtkActor* addFile(vtkRenderer* renderer, const char* filename)
{
    // add a vtk file to the scene
    const char* material = "phong";
    vtkPolyDataReader* reader = vtkPolyDataReader::New();
    reader->SetFileName(filename);

    vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
    mapper->SetInputConnection(reader->GetOutputPort());
    mapper->ScalarVisibilityOff();

    vtkActor* actor = vtkActor::New();
    actor->SetMapper(mapper);

    vtkProperty* property = actor->GetProperty();
    property->SetDiffuseColor(0.73, 0.9383, 0.2739);
    property->SetSpecularColor(0.5, 0.5, 0.5);
    property->SetSpecular(0.2);
    property->SetSpecularPower(100);

#ifdef USE_MANTA
    vtkMantaProperty* mantaProperty = vtkMantaProperty::SafeDownCast(
                                                     actor->GetProperty());
    mantaProperty->SetMaterialType(material);
#endif

    renderer->AddActor(actor);
    return actor;
}

void addLights(vtkRenderer* renderer)
{
    // light in front of camera
    vtkLight *light1 = vtkLight::New();
    light1->PositionalOn();
    light1->SetPosition(renderer->GetActiveCamera()->GetPosition());
    light1->SetFocalPoint(renderer->GetActiveCamera()->GetFocalPoint());
    light1->SetColor(0.5, 0.5, 0.5);
    light1->SetLightTypeToCameraLight();
    renderer->SetLightFollowCamera(1);
    renderer->AddLight(light1);

    // light in upper right
    vtkLight *light2 = vtkLight::New();
    light2->PositionalOn();
    light2->SetPosition(5, 5, 5);
    light2->SetColor(0.5, 0.5, 0.5);
    light2->SetLightTypeToCameraLight();
    renderer->SetLightFollowCamera(1);
    renderer->AddLight(light2);

    // light straight up, looking down
    vtkLight *light3 = vtkLight::New();
    light3->PositionalOn();
    light3->SetPosition(0.5, 5, 0.5);
    light3->SetFocalPoint(0, 0, 0);
    light3->SetColor(0.5, 0.5, 0.5);
    light3->SetLightTypeToCameraLight();
    renderer->SetLightFollowCamera(1);
    renderer->AddLight(light3);

    // light looking down the z-axis
    vtkLight *light4 = vtkLight::New();
    light4->PositionalOn();
    light4->SetPosition(-5, 0, 0);
    light4->SetFocalPoint(0, 0, 0);
    light4->SetColor(0.4, 0.4, 0.4);
    light4->SetLightTypeToCameraLight();
    renderer->SetLightFollowCamera(1);
    renderer->AddLight(light4);
}

