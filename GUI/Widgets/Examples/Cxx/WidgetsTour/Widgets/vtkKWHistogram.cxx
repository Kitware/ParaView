#include "vtkPiecewiseFunction.h"
#include "vtkKWApplication.h"
#include "vtkKWPiecewiseFunctionEditor.h"
#include "vtkKWHistogram.h"
#include "vtkImageData.h"
#include "vtkImageReslice.h"
#include "vtkPointData.h"
#include "vtkKWLabel.h"
#include "vtkKWWindow.h"
#include "vtkXMLImageDataReader.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWHistogramItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *);
};

void vtkKWHistogramItem::Create(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // The histograms are based on a real image data. Let's load it first

  vtkXMLImageDataReader *pfed_reader = vtkXMLImageDataReader::New();
  pfed_reader->SetFileName(
    vtkKWWidgetsTourExample::GetPathToExampleData(app, "head100x100x47.vti"));

  // Build the histogram 

  pfed_reader->Update();
  vtkKWHistogram *pfed_hist = vtkKWHistogram::New();
  pfed_hist->BuildHistogram(
    pfed_reader->GetOutput()->GetPointData()->GetScalars(), 0);

  // Create a transfer function editor to display the histogram

  vtkKWPiecewiseFunctionEditor *pfed_tfunc1_editor = 
    vtkKWPiecewiseFunctionEditor::New();
  pfed_tfunc1_editor->SetParent(parent);
  pfed_tfunc1_editor->Create();
  pfed_tfunc1_editor->SetBalloonHelpString(
    "This histogram is displayed using a transfer function editor.");

  pfed_tfunc1_editor->SetHistogram(pfed_hist);
  pfed_tfunc1_editor->DisplayHistogramOnly();

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2 -pady 10", 
    pfed_tfunc1_editor->GetWidgetName());

  // -----------------------------------------------------------------------
  
  // Let's display 2 histograms at the same time

  vtkImageReslice *pfed_img_reslice = vtkImageReslice::New();
  pfed_img_reslice->SetInput(pfed_reader->GetOutput());
  int *ext = pfed_img_reslice->GetInput()->GetWholeExtent();
  int mid =  ext[5] / 2;
  pfed_img_reslice->SetOutputExtent(
    ext[0], ext[1], 
    ext[2], ext[3], 
    mid, mid);

  // Build the histogram for one slice that was extracted using vtkImageReslice

  pfed_img_reslice->Update();
  vtkKWHistogram *pfed_hist2 = vtkKWHistogram::New();
  pfed_hist2->BuildHistogram(
    pfed_img_reslice->GetOutput()->GetPointData()->GetScalars(), 0);

  // Create a transfer function editor to display both histograms

  vtkKWPiecewiseFunctionEditor *pfed_tfunc2_editor = 
    vtkKWPiecewiseFunctionEditor::New();
  pfed_tfunc2_editor->SetParent(parent);
  pfed_tfunc2_editor->Create();
  pfed_tfunc2_editor->SetBalloonHelpString(
    "This transfer function editor is used to display two histograms at "
    "a time. One of them is displayed as bars, the second as a polyline. "
    "The range slider can be used to zoom and pan, ticks are "
    "displayed on each side, and the default colors are changed.");
  
  pfed_tfunc2_editor->SetHistogram(pfed_hist);
  pfed_tfunc2_editor->SetSecondaryHistogram(pfed_hist2);
  pfed_tfunc2_editor->DisplayHistogramOnly();

  // Display the ticks

  pfed_tfunc2_editor->ParameterTicksVisibilityOn();
  pfed_tfunc2_editor->ValueTicksVisibilityOn();
  pfed_tfunc2_editor->ComputeValueTicksFromHistogramOn();
  pfed_tfunc2_editor->SetParameterTicksFormat("%-#6.0f");
  pfed_tfunc2_editor->SetValueTicksFormat(
    pfed_tfunc2_editor->GetParameterTicksFormat());

  // Change the size, show the range slider, the label, and change some colors

  pfed_tfunc2_editor->ParameterRangeVisibilityOn();
  pfed_tfunc2_editor->SetCanvasWidth(450);
  pfed_tfunc2_editor->SetCanvasHeight(150);
  pfed_tfunc2_editor->SetLabelText("Histogram:");
  pfed_tfunc2_editor->LabelVisibilityOn();
  pfed_tfunc2_editor->SetFrameBackgroundColor(0.92, 1.0, 0.92);

    // Change the histogram draw style

  pfed_tfunc2_editor->SetHistogramColor(0.18, 0.24, 0.49);
  pfed_tfunc2_editor->SetHistogramStyleToBars();

  pfed_tfunc2_editor->SetSecondaryHistogramColor(1.0, 0.97, 0.3);
  pfed_tfunc2_editor->SetSecondaryHistogramStyleToPolyLine();

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 20", 
    pfed_tfunc2_editor->GetWidgetName());

  pfed_img_reslice->Delete();
  pfed_tfunc1_editor->Delete();
  pfed_tfunc2_editor->Delete();
  pfed_hist->Delete();
  pfed_hist2->Delete();
  pfed_reader->Delete();
}

int vtkKWHistogramItem::GetType()
{
  return KWWidgetsTourItem::TypeVTK;
}

KWWidgetsTourItem* vtkKWHistogramEntryPoint()
{
  return new vtkKWHistogramItem();
}
