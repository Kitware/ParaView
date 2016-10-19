#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

#include "vtkAppendPolyData.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkMantaActor.h"
#include "vtkMantaCamera.h"
#include "vtkMantaLight.h"
#include "vtkMantaPolyDataMapper.h"
#include "vtkMantaProperty.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaRenderer.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkRegressionTestImage.h"

// this program tests
//
// * rendering of hybrid vertices+lines vtkpolyData objects
//
// * image compositing between multiple renderers on one or two layers

double blue[] = { 0.0000, 0.0000, 0.8000 };
double black[] = { 0.0000, 0.0000, 0.0000 };
double banana[] = { 0.8900, 0.8100, 0.3400 };
double tomato[] = { 1.0000, 0.3882, 0.2784 };

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int invalidCmd = 0;

  if (argc < 2)
  {
    invalidCmd = 1;
  }

#ifndef _RECORD_EVENTS_
  if (strstr(argv[1], "multiRensEvents.log"))
  {
    FILE* file = NULL;
    file = fopen(argv[1], "r");
    if (file == NULL)
    {
      invalidCmd = 1;
    }
    fclose(file);
  }
#endif

  if (invalidCmd)
  {
    cerr << argv[0] << " ${vtkManta_source_directory}"
         << "/examples/multiRensEvents.log [-I]" << endl;
    return 1;
  }

  int i;

  // the smaller cube (surface)
  static double cord1[8][3] = { { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f },
    { 0.5f, 0.5f, -0.5f }, { -0.5f, 0.5f, -0.5f }, { -0.5f, -0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f },
    { 0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, 0.5f } };

  static vtkIdType quads[6][4] = { { 0, 1, 2, 3 }, { 4, 5, 6, 7 }, { 0, 1, 5, 4 }, { 1, 2, 6, 5 },
    { 2, 3, 7, 6 }, { 3, 0, 4, 7 } };

  vtkPoints* pnts = vtkPoints::New();
  vtkPolyData* cube = vtkPolyData::New();
  vtkCellArray* surf = vtkCellArray::New();
  vtkFloatArray* vals = vtkFloatArray::New();

  for (i = 0; i < 8; i++)
    pnts->InsertPoint(i, cord1[i]);
  for (i = 0; i < 6; i++)
    surf->InsertNextCell(4, quads[i]);
  for (i = 0; i < 8; i++)
    vals->InsertTuple1(i, i);

  cube->SetPoints(pnts);
  pnts->Delete();
  cube->SetPolys(surf);
  surf->Delete();
  cube->GetPointData()->SetActiveAttribute(
    cube->GetPointData()->SetScalars(vals), vtkDataSetAttributes::SCALARS);
  vals->Delete();

  // the bigger cube (edges / lines)
  static double cord2[8][3] = { { -1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f },
    { 1.0f, 1.0f, -1.0f }, { -1.0f, 1.0f, -1.0f }, { -1.0f, -1.0f, 1.0f }, { 1.0f, -1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f } };

  vtkPoints* p_pnt = vtkPoints::New();
  vtkIdList* edges = vtkIdList::New();
  vtkPolyData* lines = vtkPolyData::New();
  vtkCellArray* line2 = vtkCellArray::New();

  for (i = 0; i < 4; i++)
  {
    p_pnt->InsertPoint(i, cord2[i]);
    edges->InsertNextId(i);
  }
  edges->InsertNextId(0);
  line2->InsertNextCell(edges);
  edges->Reset();

  for (i = 4; i < 8; i++)
  {
    p_pnt->InsertPoint(i, cord2[i]);
    edges->InsertNextId(i);
  }
  edges->InsertNextId(4);
  line2->InsertNextCell(edges);
  edges->Reset();

  edges->InsertNextId(0);
  edges->InsertNextId(4);
  line2->InsertNextCell(edges);
  edges->Reset();
  edges->InsertNextId(5);
  edges->InsertNextId(1);
  line2->InsertNextCell(edges);
  edges->Reset();

  edges->InsertNextId(2);
  edges->InsertNextId(6);
  line2->InsertNextCell(edges);
  edges->Reset();
  edges->InsertNextId(3);
  edges->InsertNextId(7);
  line2->InsertNextCell(edges);

  lines->SetPoints(p_pnt);
  p_pnt->Delete();
  lines->SetLines(line2);
  line2->Delete();
  edges->Delete();

  // eight vertices outside the smaller cube
  double ptCrd[8][3] = { { -0.75f, -0.75f, -0.75f }, { 0.75f, -0.75f, -0.75f },
    { 0.75f, 0.75f, -0.75f }, { -0.75f, 0.75f, -0.75f }, { -0.75f, -0.75f, 0.75f },
    { 0.75f, -0.75f, 0.75f }, { 0.75f, 0.75f, 0.75f }, { -0.75f, 0.75f, 0.75f } };

  vtkIdType pt_id[8][1] = { { 0 }, { 1 }, { 2 }, { 3 }, { 4 }, { 5 }, { 6 }, { 7 } };

  vtkPoints* ponts = vtkPoints::New();
  vtkCellArray* verts = vtkCellArray::New();
  for (i = 0; i < 8; i++)
    ponts->InsertPoint(i, ptCrd[i]);
  for (i = 0; i < 8; i++)
    verts->InsertNextCell(1, pt_id[i]);
  vtkPolyData* vPoly = vtkPolyData::New();
  vPoly->SetPoints(ponts);
  ponts->Delete();
  vPoly->SetVerts(verts);
  verts->Delete();

  // hybrid vertices+lines polydata
  vtkAppendPolyData* apend = vtkAppendPolyData::New();
  apend->AddInput(vPoly);
  apend->AddInput(lines);

  // hybrid mapper and actor
  vtkMantaPolyDataMapper* hybridMapper = vtkMantaPolyDataMapper::New();
  hybridMapper->SetInputConnection(apend->GetOutputPort());
  hybridMapper->ScalarVisibilityOff();
  vtkMantaActor* hybridActor = vtkMantaActor::New();
  hybridActor->SetMapper(hybridMapper);
  hybridActor->GetProperty()->SetDiffuseColor(tomato);
  hybridActor->GetProperty()->SetSpecular(0.4);
  hybridActor->GetProperty()->SetSpecularPower(10);
  hybridActor->GetProperty()->SetLineWidth(16.0);
  hybridActor->GetProperty()->SetPointSize(32.0);

  // cube mapper and actor
  vtkMantaPolyDataMapper* cubeMapper = vtkMantaPolyDataMapper::New();
  cubeMapper->SetInput(cube);
  cubeMapper->ScalarVisibilityOff();
  vtkMantaActor* cubeActor = vtkMantaActor::New();
  cubeActor->SetMapper(cubeMapper);
  cubeActor->GetProperty()->SetDiffuseColor(banana);
  cubeActor->GetProperty()->SetSpecular(0.4);
  cubeActor->GetProperty()->SetSpecularPower(10);
  cubeActor->GetProperty()->SetOpacity(1.0);

  // add a light for shadows
  vtkMantaLight* light = vtkMantaLight::New();
  light->SetLightTypeToCameraLight();
  vtkMantaCamera* camera = vtkMantaCamera::New();
  vtkMantaRenderer* cubeRen = vtkMantaRenderer::New();

  vtkMantaRenderer* hybridRen = vtkMantaRenderer::New();
  hybridRen->SetViewport(0.0, 0.0, 1.0, 1.0);
  hybridRen->SetLayer(0);
  hybridRen->SetBackground(blue[0], blue[1], blue[2]);
  hybridRen->AddActor(hybridActor);

  cubeRen->SetViewport(0.0, 0.0, 0.6, 1.0);
  cubeRen->SetLayer(1);
  cubeRen->InteractiveOff();
  cubeRen->SetBackground(black[0], black[1], black[2]);
  cubeRen->AddActor(cubeActor);

  cubeRen->SetLightFollowCamera(1);
  cubeRen->AddLight(light);
  cubeRen->SetActiveCamera(camera);
  cubeRen->ResetCameraClippingRange();
  cubeRen->GetActiveCamera()->Dolly(1.2);
  cubeRen->GetActiveCamera()->Azimuth(30);
  cubeRen->GetActiveCamera()->Elevation(20);
  cubeRen->ResetCamera();

  vtkRenderWindow* renWin = vtkRenderWindow::New();

  renWin->SetNumberOfLayers(2);
  renWin->AddRenderer(hybridRen);

  renWin->AddRenderer(cubeRen);
  renWin->SetSize(800, 400);

  vtkRenderWindowInteractor* interactor = vtkRenderWindowInteractor::New();
  interactor->SetRenderWindow(renWin);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  else
  {
    vtkInteractorEventRecorder* recorder = vtkInteractorEventRecorder::New();
    recorder->SetInteractor(interactor);
    recorder->SetFileName(argv[1]);

#ifdef _RECORD_EVENTS_
    recorder->SetEnabled(true);
    recorder->Record();
    renWin->Render();
    interactor->Start();
    recorder->SetEnabled(false);
#else
    renWin->Render();
    recorder->Play();
    retVal = vtkRegressionTestImage(renWin);
    recorder->Off();
#endif

    recorder->Delete();
  }

  // memory deallocation
  cube->Delete();
  lines->Delete();
  vPoly->Delete();
  apend->Delete();

  hybridMapper->Delete();
  hybridActor->Delete();
  cubeMapper->Delete();
  cubeActor->Delete();

  light->Delete();
  camera->Delete();

  renWin->RemoveRenderer(cubeRen);
  renWin->RemoveRenderer(hybridRen);

  cubeRen->Delete();

  hybridRen->Delete();

  renWin->Delete();
  interactor->Delete();

  return !retVal;
}
