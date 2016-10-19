#include "vtkCleanUnstructuredGrid.h"
#include "vtkDataSetReader.h"
#include "vtkExtractHistogram.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTransferFunctionEditorWidget.h"
#include "vtkTransferFunctionViewer.h"

int main(int argc, char* argv[])
{
  vtkTesting* t = vtkTesting::New();
  int cc;
  for (cc = 1; cc < argc; cc++)
  {
    t->AddArgument(argv[cc]);
  }

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ironProt.vtk");

  vtkDataSetReader* reader = vtkDataSetReader::New();
  reader->SetFileName(fname);
  delete[] fname;

  vtkCleanUnstructuredGrid* clean = vtkCleanUnstructuredGrid::New();
  clean->SetInputConnection(reader->GetOutputPort());

  vtkExtractHistogram* histogram = vtkExtractHistogram::New();
  histogram->SetInputConnection(clean->GetOutputPort());
  histogram->SetComponent(0);
  histogram->SetBinCount(256);
  histogram->Update();

  vtkPiecewiseFunction* oFunc = vtkPiecewiseFunction::New();
  oFunc->AddPoint(0, 0.0);
  oFunc->AddPoint(255, 0.2);

  vtkTransferFunctionViewer* viewer = vtkTransferFunctionViewer::New();
  viewer->SetTransferFunctionEditorTypeToSimple1D();
  viewer->SetModificationTypeToOpacity();
  // viewer->SetHistogram(histogram->GetOutput());
  // viewer->SetHistogramVisibility(1);
  viewer->SetHistogramColor(0.67, 0.82, 1.0);
  viewer->SetElementsColor(1, 0, 1);
  viewer->SetAllowInteriorElements(0);
  viewer->SetOpacityFunction(oFunc);
  viewer->SetVisibleScalarRange(100, 200);
  viewer->SetSize(800, 300);
  viewer->Render();

  viewer->GetEditorWidget()->ShowWholeScalarRange();
  viewer->Render();

  t->SetRenderWindow(viewer->GetRenderWindow());
  int retVal = t->RegressionTest(10);

  histogram->Delete();
  reader->Delete();
  clean->Delete();
  viewer->Delete();
  oFunc->Delete();
  t->Delete();

  return !retVal;
}
