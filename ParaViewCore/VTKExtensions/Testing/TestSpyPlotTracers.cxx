#include "vtkDummyController.h"
#include "vtkSmartPointer.h"
#include "vtkSpyPlotReader.h"
#include "vtkTestUtilities.h"

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestSpyPlotTracers(int argc, char* argv[])
{
  char* fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Testing/Data/SPCTH/ball_and_box.spcth");

  VTK_CREATE(vtkDummyController, controller);
  vtkMultiProcessController::SetGlobalController(controller);

  VTK_CREATE(vtkSpyPlotReader, reader);
  reader->SetGlobalController(controller);
  reader->SetFileName(fname);
  reader->GenerateTracerArrayOn();
  reader->Update();

  delete[] fname;

  return 0;
}
