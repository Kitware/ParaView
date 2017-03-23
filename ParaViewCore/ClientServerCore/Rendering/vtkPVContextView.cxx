/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContextView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVContextView.h"

#include "vtkAnnotationLink.h"
#include "vtkCSVExporter.h"
#include "vtkCamera.h"
#include "vtkChart.h"
#include "vtkChartRepresentation.h"
#include "vtkCommand.h"
#include "vtkContextView.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVContextInteractorStyle.h"
#include "vtkPVDataRepresentation.h"
#include "vtkPVOptions.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkProcessModule.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScatterPlotMatrix.h"
#include "vtkSelection.h"
#include "vtkTileDisplayHelper.h"
#include "vtkTilesHelper.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkPVContextView::vtkPVContextView()
  : InteractorStyle()
{
  vtkPVOptions* options = vtkProcessModule::GetProcessModule()
    ? vtkProcessModule::GetProcessModule()->GetOptions()
    : NULL;

  this->UseOffscreenRenderingForScreenshots = false;
  this->UseOffscreenRendering = (options ? options->GetUseOffscreenRendering() != 0 : false);

  this->RenderWindow = this->SynchronizedWindows->NewRenderWindow();
  this->RenderWindow->SetOffScreenRendering(this->UseOffscreenRendering ? 1 : 0);
  this->ContextView = vtkContextView::New();

  // Let the application setup the interactor.
  this->ContextView->SetRenderWindow(this->RenderWindow);
  if (this->ContextView->GetInteractor())
  {
    this->ContextView->GetInteractor()->SetInteractorStyle(NULL);
  }
  this->ContextView->SetInteractor(NULL);
  this->ContextView->GetRenderer()->AddObserver(
    vtkCommand::StartEvent, this, &vtkPVContextView::OnStartRender);
  this->ContextView->GetRenderer()->AddObserver(
    vtkCommand::EndEvent, this, &vtkPVContextView::OnEndRender);
}

//----------------------------------------------------------------------------
vtkPVContextView::~vtkPVContextView()
{
  vtkTileDisplayHelper::GetInstance()->EraseTile(this->Identifier);

  this->RenderWindow->Delete();
  this->ContextView->Delete();
}

//----------------------------------------------------------------------------
void vtkPVContextView::Initialize(unsigned int id)
{
  if (this->Identifier == id)
  {
    // already initialized
    return;
  }
  this->SynchronizedWindows->AddRenderWindow(id, this->RenderWindow);
  this->SynchronizedWindows->AddRenderer(id, this->ContextView->GetRenderer());
  this->Superclass::Initialize(id);
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
void vtkPVContextView::SetUseOffscreenRendering(bool use_offscreen)
{
  if (this->UseOffscreenRendering == use_offscreen)
  {
    return;
  }

  vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();
  bool process_use_offscreen = options->GetUseOffscreenRendering() != 0;

  this->UseOffscreenRendering = use_offscreen || process_use_offscreen;
  this->GetRenderWindow()->SetOffScreenRendering(this->UseOffscreenRendering);
}

//----------------------------------------------------------------------------
void vtkPVContextView::Update()
{
  vtkMultiProcessController* s_controller = this->SynchronizedWindows->GetClientServerController();
  vtkMultiProcessController* d_controller =
    this->SynchronizedWindows->GetClientDataServerController();
  vtkMultiProcessController* p_controller = vtkMultiProcessController::GetGlobalController();
  vtkMultiProcessStream stream;

  if (this->SynchronizedWindows->GetLocalProcessIsDriver())
  {
    std::vector<int> need_delivery;
    int num_reprs = this->GetNumberOfRepresentations();
    for (int cc = 0; cc < num_reprs; cc++)
    {
      vtkPVDataRepresentation* repr =
        vtkPVDataRepresentation::SafeDownCast(this->GetRepresentation(cc));
      if (repr && repr->GetNeedUpdate())
      {
        need_delivery.push_back(cc);
      }
    }
    stream << static_cast<int>(need_delivery.size());
    for (size_t cc = 0; cc < need_delivery.size(); cc++)
    {
      stream << need_delivery[cc];
    }

    if (s_controller)
    {
      s_controller->Send(stream, 1, 998878);
    }
    if (d_controller)
    {
      d_controller->Send(stream, 1, 998878);
    }
    if (p_controller)
    {
      p_controller->Broadcast(stream, 0);
    }
  }
  else
  {
    if (s_controller)
    {
      s_controller->Receive(stream, 1, 998878);
    }
    if (d_controller)
    {
      d_controller->Receive(stream, 1, 998878);
    }
    if (p_controller)
    {
      p_controller->Broadcast(stream, 0);
    }
  }

  int size;
  stream >> size;
  for (int cc = 0; cc < size; cc++)
  {
    int index;
    stream >> index;
    vtkPVDataRepresentation* repr =
      vtkPVDataRepresentation::SafeDownCast(this->GetRepresentation(index));
    if (repr)
    {
      repr->MarkModified();
    }
  }
  this->Superclass::Update();
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
  this->SynchronizedWindows->SetEnabled(this->InTileDisplayMode());
  this->SynchronizedWindows->BeginRender(this->GetIdentifier());

  // Call Render() on local render window only on the client (or root node in
  // batch mode).
  //
  // In symmetric mode, we call render on all ranks for one reason only:
  // when the vtkWindowToImageFilter tries to grab frame buffers on the
  // satellites, it doesn't die. In reality, we shouldn't grab the frame buffers
  // at all on satellites at all, but that needs some refactoring in
  // vtkWindowToImageFilter.
  if (this->SynchronizedWindows->GetLocalProcessIsDriver() || this->InTileDisplayMode() ||
    vtkProcessModule::GetProcessModule()->GetSymmetricMPIMode())
  {
    vtkTimerLog::MarkStartEvent("vtkPVContextView::PrepareForRender");
    // on rendering-nodes call Render-pass so that representations can update the
    // vtk-charts as needed.
    this->CallProcessViewRequest(
      vtkPVView::REQUEST_RENDER(), this->RequestInformation, this->ReplyInformationVector);
    vtkTimerLog::MarkEndEvent("vtkPVContextView::PrepareForRender");

    this->ContextView->Render();
  }
  this->SynchronizedWindows->SetEnabled(false);
}

//----------------------------------------------------------------------------
void vtkPVContextView::OnStartRender()
{
  vtkTileDisplayHelper::GetInstance()->EraseTile(
    this->Identifier, this->ContextView->GetRenderer()->GetActiveCamera()->GetLeftEye());
}

//----------------------------------------------------------------------------
void vtkPVContextView::OnEndRender()
{
  if (this->SynchronizedWindows->GetLocalProcessIsDriver() || !this->InTileDisplayMode())
  {
    return;
  }

  // this code needs to be called on only server-nodes in tile-display mode.

  double viewport[4];
  this->ContextView->GetRenderer()->GetViewport(viewport);

  int tile_dims[2], tile_mullions[2];
  this->SynchronizedWindows->GetTileDisplayParameters(tile_dims, tile_mullions);

  double tile_viewport[4];
  this->GetRenderWindow()->GetTileViewport(tile_viewport);

  double physical_viewport[4];
  vtkSmartPointer<vtkTilesHelper> tilesHelper = vtkSmartPointer<vtkTilesHelper>::New();
  tilesHelper->SetTileDimensions(tile_dims);
  tilesHelper->SetTileMullions(tile_mullions);
  tilesHelper->SetTileWindowSize(this->GetRenderWindow()->GetActualSize());
  if (tilesHelper->GetPhysicalViewport(viewport,
        vtkMultiProcessController::GetGlobalController()->GetLocalProcessId(), physical_viewport))
  {
    // When tiling, vtkContextActor renders the result at the
    // "physical_viewport" location on the window. So we grab the image only
    // from that section of the view.
    vtkSynchronizedRenderers::vtkRawImage image;
    this->ContextView->GetRenderer()->SetViewport(physical_viewport);
    image.Capture(this->ContextView->GetRenderer());
    this->ContextView->GetRenderer()->SetViewport(viewport);

    vtkTileDisplayHelper::GetInstance()->SetTile(
      this->Identifier, physical_viewport, this->ContextView->GetRenderer(), image);
  }

  vtkTileDisplayHelper::GetInstance()->FlushTiles(
    this->Identifier, this->ContextView->GetRenderer()->GetActiveCamera()->GetLeftEye());
}

//----------------------------------------------------------------------------
template <class T>
vtkSelection* vtkPVContextView::GetSelectionImplementation(T* chart)
{
  if (vtkSelection* selection = chart->GetAnnotationLink()->GetCurrentSelection())
  {
    if (this->SelectionClone == NULL || this->SelectionClone->GetMTime() < selection->GetMTime() ||
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
  this->SelectionClone = NULL;
  return NULL;
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
  this->SelectionClone = NULL;
  return NULL;
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
bool vtkPVContextView::Export(vtkCSVExporter* exporter)
{
  exporter->Open(vtkCSVExporter::STREAM_COLUMNS);
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
void vtkPVContextView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
