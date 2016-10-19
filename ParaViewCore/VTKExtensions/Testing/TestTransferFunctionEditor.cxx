#include "vtkCleanUnstructuredGrid.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataSetReader.h"
#include "vtkExtractHistogram.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
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
  oFunc->AddPoint(20, 0.0);
  oFunc->AddPoint(255, 0.2);

  vtkColorTransferFunction* cFunc = vtkColorTransferFunction::New();
  cFunc->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  cFunc->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
  cFunc->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
  cFunc->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
  cFunc->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

  vtkTransferFunctionViewer* viewer = vtkTransferFunctionViewer::New();
  viewer->SetTransferFunctionEditorTypeToSimple1D();
  viewer->SetModificationTypeToColorAndOpacity();
  // viewer->SetHistogram(histogram->GetOutput());
  // viewer->SetHistogramVisibility(1);
  viewer->SetShowColorFunctionInHistogram(1);
  viewer->SetShowColorFunctionOnLines(0);
  viewer->SetColorElementsByColorFunction(0);
  viewer->SetElementsColor(1, 0, 1);
  viewer->SetColorSpace(1); // HSV
  viewer->SetElementLighting(0, 0.5, 0.5, 10);
  viewer->SetAllowInteriorElements(1);
  viewer->SetOpacityFunction(oFunc);
  viewer->SetColorFunction(cFunc);
  viewer->SetVisibleScalarRange(100, 200);
  viewer->SetSize(800, 300);

  viewer->SetElementOpacity(3, 0.5);
  viewer->SetElementRGBColor(3, 0.0, 0.3, 0.8);
  viewer->SetElementHSVColor(4, 0.5, 1, 1);
  viewer->SetElementScalar(4, 175.0);
  viewer->MoveToPreviousElement();
  viewer->MoveToPreviousElement();
  viewer->MoveToNextElement();
  viewer->Render();

  double color[3];
  viewer->GetElementHSVColor(2, color);
  if (color[0] != 0 || color[1] != 1 || color[2] != 1)
  {
    cout << "ERROR: Wrong HSV color reported." << endl;
    histogram->Delete();
    reader->Delete();
    clean->Delete();
    viewer->Delete();
    cFunc->Delete();
    oFunc->Delete();
    t->Delete();
    return 1;
  }

  double* range = viewer->GetVisibleScalarRange();
  if (range[0] != 100 || range[1] != 200)
  {
    cout << "ERROR: Visible scalar range incorrectly reported." << endl;
    histogram->Delete();
    reader->Delete();
    clean->Delete();
    viewer->Delete();
    cFunc->Delete();
    oFunc->Delete();
    t->Delete();
    return 1;
  }

  range = viewer->GetWholeScalarRange();
  double range2[2];
  viewer->GetWholeScalarRange(range2);
  if (range[0] != 0 || range[1] != 255 || range2[0] != 0 || range2[1] != 255)
  {
    cout << "ERROR: Whole scalar range incorrectly reported." << endl;
    histogram->Delete();
    reader->Delete();
    clean->Delete();
    viewer->Delete();
    cFunc->Delete();
    oFunc->Delete();
    t->Delete();
    return 1;
  }

  t->SetRenderWindow(viewer->GetRenderWindow());
  int retVal = t->RegressionTest(10);

  histogram->Delete();
  reader->Delete();
  clean->Delete();
  viewer->Delete();
  cFunc->Delete();
  oFunc->Delete();
  t->Delete();

  return !retVal;
}
