// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVContextView.h"

#include "vtkAbstractChartExporter.h"
#include "vtkAnnotationLink.h"
#include "vtkCSVExporter.h"
#include "vtkCamera.h"
#include "vtkChart.h"
#include "vtkChartRepresentation.h"
#include "vtkContextView.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextInteractorStyle.h"
#include "vtkPVSession.h"
#include "vtkPVStringFormatter.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkSelection.h"
#include "vtkTimerLog.h"

#include <sstream>

//----------------------------------------------------------------------------
vtkPVContextView::vtkPVContextView()
{
  this->ContextView = vtkContextView::New();

  // Let the application setup the interactor.
  this->ContextView->SetRenderWindow(this->GetRenderWindow());
  if (this->ContextView->GetInteractor())
  {
    this->ContextView->GetInteractor()->SetInteractorStyle(nullptr);
  }
  this->ContextView->SetInteractor(nullptr);
}

//----------------------------------------------------------------------------
vtkPVContextView::~vtkPVContextView()
{
  this->ContextView->Delete();
  this->SetTitle(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVContextView::SetupInteractor(vtkRenderWindowInteractor* iren)
{
  // Disable interactor on server processes (or batch processes), since
  // otherwise the vtkContextInteractorStyle triggers renders on changes to the
  // vtkContextView which is bad and can cause deadlock (BUG #122651).
  if (this->GetLocalProcessSupportsInteraction() == false)
  {
    // We don't setup interactor on non-driver processes.
    return;
  }
  this->ContextView->SetInteractor(iren);
  this->InteractorStyle->SetScene(this->ContextView->GetScene());
  if (iren)
  {
    iren->SetInteractorStyle(this->InteractorStyle.GetPointer());
  }
}

//----------------------------------------------------------------------------
vtkRenderWindowInteractor* vtkPVContextView::GetInteractor()
{
  return this->ContextView->GetInteractor();
}

//----------------------------------------------------------------------------
void vtkPVContextView::StillRender()
{
  vtkTimerLog::MarkStartEvent("Still Render");
  this->Render(false);
  vtkTimerLog::MarkEndEvent("Still Render");
}

//----------------------------------------------------------------------------
void vtkPVContextView::InteractiveRender()
{
  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->Render(true);
  vtkTimerLog::MarkEndEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkPVContextView::Render(bool vtkNotUsed(interactive))
{
  vtkTimerLog::MarkStartEvent("vtkPVContextView::PrepareForRender");
  // on rendering-nodes call Render-pass so that representations can update the
  // vtk-charts as needed.
  this->CallProcessViewRequest(
    vtkPVView::REQUEST_RENDER(), this->RequestInformation, this->ReplyInformationVector);
  vtkTimerLog::MarkEndEvent("vtkPVContextView::PrepareForRender");

  this->ContextView->Render();
}

//----------------------------------------------------------------------------
template <class T>
vtkSelection* vtkPVContextView::GetSelectionImplementation(T* chart)
{
  if (vtkSelection* selection = chart->GetAnnotationLink()->GetCurrentSelection())
  {
    if (this->SelectionClone == nullptr ||
      this->SelectionClone->GetMTime() < selection->GetMTime() ||
      this->SelectionClone->GetMTime() < chart->GetAnnotationLink()->GetMTime())
    {
      // we need to treat vtkSelection obtained from vtkAnnotationLink as
      // constant and not modify it. Hence, we create a clone.
      this->SelectionClone = vtkSmartPointer<vtkSelection>::New();
      this->SelectionClone->ShallowCopy(selection);

      // Allow the view to transform the selection as appropriate since the raw
      // selection created by the VTK view is on the "transformed" data put in
      // the view and not original input data.
      if (this->MapSelectionToInput(this->SelectionClone) == false)
      {
        this->SelectionClone->Initialize();
      }
    }
    return this->SelectionClone;
  }
  this->SelectionClone = nullptr;
  return nullptr;
}

//----------------------------------------------------------------------------
vtkSelection* vtkPVContextView::GetSelection()
{
  if (vtkChart* chart = vtkChart::SafeDownCast(this->GetContextItem()))
  {
    return this->GetSelectionImplementation(chart);
  }
  else if (vtkScatterPlotMatrix* schart =
             vtkScatterPlotMatrix::SafeDownCast(this->GetContextItem()))
  {
    return this->GetSelectionImplementation(schart);
  }

  vtkWarningMacro("Unsupported context item type.");
  this->SelectionClone = nullptr;
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkPVContextView::MapSelectionToInput(vtkSelection* sel)
{
  for (int cc = 0, max = this->GetNumberOfRepresentations(); cc < max; cc++)
  {
    vtkChartRepresentation* repr =
      vtkChartRepresentation::SafeDownCast(this->GetRepresentation(cc));
    if (repr && repr->GetVisibility() && repr->MapSelectionToInput(sel))
    {
      return true;
    }
  }
  // error! we cannot have  a selection created in the view, there's no visible
  // representation!
  return false;
}

//----------------------------------------------------------------------------
bool vtkPVContextView::Export(vtkAbstractChartExporter* exporter)
{
  exporter->Open(vtkAbstractChartExporter::STREAM_COLUMNS);
  for (int cc = 0, max = this->GetNumberOfRepresentations(); cc < max; cc++)
  {
    vtkChartRepresentation* repr =
      vtkChartRepresentation::SafeDownCast(this->GetRepresentation(cc));
    if (repr && repr->GetVisibility() && !repr->Export(exporter))
    {
      exporter->Abort();
      vtkErrorMacro("Failed to export to CSV. Exporting may not be supported by this view.");
      return false;
    }
  }
  exporter->Close();
  return true;
}

//----------------------------------------------------------------------------
bool vtkPVContextView::Export(vtkCSVExporter* exporter)
{
  return this->Export(vtkAbstractChartExporter::SafeDownCast(exporter));
}

//----------------------------------------------------------------------------
std::string vtkPVContextView::GetFormattedTitle()
{
  return vtkPVStringFormatter::Format(this->Title ? this->Title : std::string());
}

//----------------------------------------------------------------------------
void vtkPVContextView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
