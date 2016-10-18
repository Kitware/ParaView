#include "vtkActor.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDummyController.h"
#include "vtkPVAMRDualClip.h"
#include "vtkPVGeometryFilter.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSpyPlotReader.h"
#include "vtkTestUtilities.h"

int main(int argc, char* argv[])
{
  int retVal = 0;

  typedef vtkSmartPointer<vtkDataSetSurfaceFilter> vtkDataSetSurfaceFilterRefPtr;
  typedef vtkSmartPointer<vtkPVAMRDualClip> vtkPVAMRDualClipRefPtr;
  typedef vtkSmartPointer<vtkSpyPlotReader> vtkSpyPlotReaderRefPtr;
  typedef vtkSmartPointer<vtkCompositePolyDataMapper2> vtkPolyDataMapperRefPtr;
  typedef vtkSmartPointer<vtkActor> vtkActorRefPtr;
  typedef vtkSmartPointer<vtkRenderer> vtkRenderRefPtr;
  typedef vtkSmartPointer<vtkRenderWindow> vtkRenderWindowRefPtr;
  typedef vtkSmartPointer<vtkDummyController> vtkDummyControllerRefPtr;
  typedef vtkSmartPointer<vtkPVGeometryFilter> vtkPVGeometryFilterRefPtr;
  typedef vtkSmartPointer<vtkRenderWindowInteractor> vtkRenderWindowInteractorRefPtr;

  const char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SPCTH/Dave_Karelitz_Small/spcth.0");

  if (!fname)
  {
    return retVal;
  }

  vtkDummyControllerRefPtr controller(vtkDummyControllerRefPtr::New());
  vtkMultiProcessController::SetGlobalController(controller);

  vtkSpyPlotReaderRefPtr reader = vtkSpyPlotReaderRefPtr::New();
  reader->SetFileName(fname);
  reader->SetGlobalController(controller);
  reader->MergeXYZComponentsOn();
  reader->DownConvertVolumeFractionOn();
  reader->DistributeFilesOn();
  reader->SetCellArrayStatus("Material volume fraction - 3", 1);
  reader->Update();

  vtkPVAMRDualClipRefPtr filter = vtkPVAMRDualClipRefPtr::New();
  filter->SetInputConnection(reader->GetOutputPort(0));
  //   filter->SetInput(reader->GetOutputDataObject(0));
  filter->SetVolumeFractionSurfaceValue(0.1);
  filter->SetEnableMergePoints(1);
  filter->SetEnableDegenerateCells(1);
  filter->SetEnableMultiProcessCommunication(1);
  filter->AddInputCellArrayToProcess("Material volume fraction - 3");
  //   filter->Update();

  vtkPVGeometryFilterRefPtr surface(vtkPVGeometryFilterRefPtr::New());
  surface->SetUseOutline(0);
  surface->SetInputConnection(filter->GetOutputPort(0));
  //   surface->SetInput(filter->GetOutputDataObject(0));
  //   surface->Update();

  vtkPolyDataMapperRefPtr mapper(vtkPolyDataMapperRefPtr::New());
  mapper->SetInputConnection(surface->GetOutputPort());
  //   mapper->Update();

  vtkActorRefPtr actor(vtkActorRefPtr::New());
  actor->SetMapper(mapper);

  vtkRenderRefPtr renderer(vtkRenderRefPtr::New());
  renderer->AddActor(actor);
  renderer->ResetCamera();

  vtkRenderWindowRefPtr renWin(vtkRenderWindowRefPtr::New());
  renWin->AddRenderer(renderer);

  vtkRenderWindowInteractorRefPtr iren(vtkRenderWindowInteractorRefPtr::New());
  iren->SetRenderWindow(renWin);

  retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }

  return (retVal == vtkRegressionTester::PASSED) ? 0 : 1;
}
