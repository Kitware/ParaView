#include "vtkPLOT3DReader.h"
#include "vtkStructuredGridGeometryFilter.h"
#include "vtkLookupTable.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkTubeFilter.h"
#include "vtkScalarBarActor.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkToolkits.h"

#include <string>

int main()
{
    string filename = VTK_DATA_ROOT;
    vtkPLOT3DReader *pl3d = vtkPLOT3DReader::New();
    pl3d->SetXYZFileName((filename + "/Data/combxyz.bin").c_str());
    pl3d->SetQFileName((filename + "/Data/combq.bin").c_str());
    pl3d->SetScalarFunctionNumber(100);
    pl3d->SetVectorFunctionNumber(202);
    pl3d->Update();

    vtkStructuredGridGeometryFilter *plane = vtkStructuredGridGeometryFilter::New();
    plane->SetInputConnection(pl3d->GetOutputPort());
    plane->SetExtent(1, 100, 1, 100, 7, 7);

    vtkLookupTable *lut = vtkLookupTable::New();
    vtkPolyDataMapper *planeMapper = vtkPolyDataMapper::New();
    planeMapper->SetLookupTable(lut);
    planeMapper->SetInputConnection(plane->GetOutputPort());
    planeMapper->SetScalarRange(pl3d->GetOutput()->GetScalarRange());

    vtkActor *planeActor = vtkActor::New();
    planeActor->SetMapper(planeMapper);

    // This creates an outline around the data.
    vtkStructuredGridOutlineFilter *outline = vtkStructuredGridOutlineFilter::New();
    outline->SetInputConnection(pl3d->GetOutputPort());
    // use a TubeFilter to convert lines into tubes
    vtkTubeFilter *outlineTube = vtkTubeFilter::New();
    outlineTube->SetInputConnection(outline->GetOutputPort());
    outlineTube->SetRadius(0.05);
    outlineTube->SetNumberOfSides(6);
    outlineTube->UseDefaultNormalOn();
    outlineTube->SetDefaultNormal(0.577, 0.577, 0.577);

    vtkPolyDataMapper *outlineMapper = vtkPolyDataMapper::New();
    outlineMapper->SetInputConnection(outlineTube->GetOutputPort());
    vtkActor *outlineActor = vtkActor::New();
    outlineActor->SetMapper(outlineMapper);

    lut->SetNumberOfColors(256);
    lut->Build();
    for (int i = 0; i < 16; i++) {
    lut->SetTableValue(i*16,   1, 0, 0, 1);
    lut->SetTableValue(i*16+1, 0, 1, 0, 1);
    lut->SetTableValue(i*16+2, 0, 0, 1, 1);
    lut->SetTableValue(i*16+3, 0, 0, 0, 1);
    }

    vtkScalarBarActor *scalarBar = vtkScalarBarActor::New();
    scalarBar->SetLookupTable(planeMapper->GetLookupTable());
    scalarBar->SetTitle("Temperature");
    scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    scalarBar->GetPositionCoordinate()->SetValue(0.1, 0.01);
    scalarBar->SetOrientationToHorizontal();
    scalarBar->SetWidth(0.8);
    scalarBar->SetHeight(0.17);

    vtkRenderer *ren = vtkRenderer::New();
    vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(ren);
    vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

    ren->AddActor(outlineActor);
    ren->AddActor(planeActor);
    ren->AddActor2D(scalarBar);

    ren->SetBackground(0.1, 0.2, 0.4);
    ren->TwoSidedLightingOff();

    renWin->SetSize(640, 480);

    vtkCamera *cam1 = vtkCamera::New();
    cam1->SetClippingRange(3.95297, 50);
    cam1->SetFocalPoint(8.88908, 0.595038, 29.3342);
    cam1->SetPosition(-12.3332, 31.7479, 41.2387);
    cam1->SetViewUp(0.060772, -0.319905, 0.945498);
    ren->SetActiveCamera(cam1);
    ren->ResetCamera();

    iren->Initialize();
    renWin->Render();
    iren->Start();

    pl3d->Delete();
    plane->Delete();
    lut->Delete();
    planeMapper->Delete();
    planeActor->Delete();
    outline->Delete();
    outlineTube->Delete();
    outlineMapper->Delete();
    outlineActor->Delete();
    scalarBar->Delete();

    ren->Delete();
    renWin->Delete();
    iren->Delete();
}
