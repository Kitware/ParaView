/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderView.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkRenderer.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkWeakPointer.h"

#include "vtkActor.h"
#include "vtkProperty.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDistributedDataFilter.h"
#include "vtkMultiProcessController.h"
#include "vtkPieceScalars.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"
#include "vtkScalarBarActor.h"
#include "vtkLookupTable.h"

#include "vtkProcessModule.h"
#include "vtkRemoteConnection.h"

#include <assert.h>

namespace
{
  static vtkWeakPointer<vtkPVSynchronizedRenderWindows> SynchronizedWindows;

  //-----------------------------------------------------------------------------
  void Create2DPipeline(vtkRenderer* ren)
    {
    vtkScalarBarActor* repr = vtkScalarBarActor::New();
    vtkLookupTable* lut = vtkLookupTable::New();
    lut->Build();
    repr->SetLookupTable(lut);
    lut->Delete();
    ren->AddActor(repr);
    repr->Delete();
    }
  //-----------------------------------------------------------------------------
  void CreatePipeline(vtkMultiProcessController* controller, vtkRenderer* renderer)
    {
    int num_procs = controller->GetNumberOfProcesses();
    int my_id = controller->GetLocalProcessId();

    vtkSphereSource* sphere = vtkSphereSource::New();
    sphere->SetPhiResolution(100);
    sphere->SetThetaResolution(100);

    vtkDistributedDataFilter* d3 = vtkDistributedDataFilter::New();
    d3->SetInputConnection(sphere->GetOutputPort());
    d3->SetController(controller);
    d3->SetBoundaryModeToSplitBoundaryCells();
    d3->UseMinimalMemoryOff();

    vtkDataSetSurfaceFilter* surface = vtkDataSetSurfaceFilter::New();
    surface->SetInputConnection(d3->GetOutputPort());

    vtkPieceScalars *piecescalars = vtkPieceScalars::New();
    piecescalars->SetInputConnection(surface->GetOutputPort());
    piecescalars->SetScalarModeToCellData();

    vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
    mapper->SetInputConnection(piecescalars->GetOutputPort());
    mapper->SetScalarModeToUseCellFieldData();
    mapper->SelectColorArray("Piece");
    mapper->SetScalarRange(0, num_procs-1);
    mapper->SetPiece(my_id);
    mapper->SetNumberOfPieces(num_procs);
    mapper->Update();

    //this->KdTree = d3->GetKdtree();

    vtkActor* actor = vtkActor::New();
    actor->SetMapper(mapper);
    //actor->GetProperty()->SetOpacity(this->UseOrderedCompositing? 0.5 : 1.0);
    //actor->GetProperty()->SetOpacity(0.5);
    renderer->AddActor(actor);

    actor->Delete();
    mapper->Delete();
    piecescalars->Delete();
    surface->Delete();
    d3->Delete();
    sphere->Delete();
    }
};

vtkStandardNewMacro(vtkPVRenderView);
//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
{
  this->StillRenderImageReductionFactor = 1;
  this->InteractiveRenderImageReductionFactor = 2;
  this->GeometrySize = 0;
  this->RemoteRenderingThreshold = 0;

  if (::SynchronizedWindows == NULL)
    {
    this->SynchronizedWindows = vtkPVSynchronizedRenderWindows::New();
    ::SynchronizedWindows = this->SynchronizedWindows;
    }
  else
    {
    this->SynchronizedWindows = ::SynchronizedWindows;
    this->SynchronizedWindows->Register(this);
    }

  this->SynchronizedRenderers = vtkPVSynchronizedRenderer::New();

  vtkRenderWindow* window = this->SynchronizedWindows->NewRenderWindow();
  this->RenderView = vtkRenderViewBase::New();
  this->RenderView->SetRenderWindow(window);
  window->Delete();

  this->Identifier = 0;

  this->NonCompositedRenderer = vtkRenderer::New();
  this->NonCompositedRenderer->EraseOff();
  this->NonCompositedRenderer->InteractiveOff();
  this->NonCompositedRenderer->SetLayer(2);
  this->NonCompositedRenderer->SetActiveCamera(
    this->RenderView->GetRenderer()->GetActiveCamera());
  window->AddRenderer(this->NonCompositedRenderer);
  window->SetNumberOfLayers(3);

  // We don't add the LabelRenderer.

  // DUMMY SPHERE FOR TESTING
  ::CreatePipeline(vtkMultiProcessController::GetGlobalController(),
    this->RenderView->GetRenderer());

  ::Create2DPipeline(this->NonCompositedRenderer);
}

//----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView()
{
  this->SynchronizedWindows->RemoveAllRenderers(this->Identifier);
  this->SynchronizedWindows->RemoveRenderWindow(this->Identifier);
  this->SynchronizedWindows->Delete();
  this->SynchronizedRenderers->Delete();
  this->NonCompositedRenderer->Delete();
  this->RenderView->Delete();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Initialize(unsigned int id)
{
  assert(this->Identifier == 0 && id != 0);

  this->Identifier = id;
  this->SynchronizedWindows->AddRenderWindow(id, this->RenderView->GetRenderWindow());
  this->SynchronizedWindows->AddRenderer(id, this->RenderView->GetRenderer());
  this->SynchronizedWindows->AddRenderer(id, this->GetNonCompositedRenderer());
  this->SynchronizedRenderers->SetRenderer(this->RenderView->GetRenderer());
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetPosition(int x, int y)
{
  assert(this->Identifier != 0);
  this->SynchronizedWindows->SetWindowPosition(this->Identifier, x, y);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::SetSize(int x, int y)
{
  assert(this->Identifier != 0);
  this->SynchronizedWindows->SetWindowSize(this->Identifier, x, y);
}

//----------------------------------------------------------------------------
vtkCamera* vtkPVRenderView::GetActiveCamera()
{
  return this->RenderView->GetRenderer()->GetActiveCamera();
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkPVRenderView::GetRenderWindow()
{
  return this->RenderView->GetRenderWindow();
}

//----------------------------------------------------------------------------
// Note this is called on all processes.
void vtkPVRenderView::ResetCamera()
{
  double bounds[6];
  this->SynchronizedRenderers->ComputeVisiblePropBounds(bounds);
  // Remember, vtkRenderer::ResetCamera() call
  // vtkRenderer::ResetCameraClippingPlanes() with the given bounds.
  this->RenderView->GetRenderer()->ResetCamera(bounds);
}

//----------------------------------------------------------------------------
void vtkPVRenderView::StillRender()
{
  vtkTimerLog::MarkStartEvent("Still Render");

  // Update all representations.
  // This should update mostly just the inputs to the representations, and maybe
  // the internal geometry filter.
  this->Update();

  // Gather information about geometry sizes from all representations.
  this->GatherGeometrySizeInformation();

  // Decide if we are doing remote rendering or local rendering.
  bool use_distributed_rendering = this->GetUseDistributedRendering();
  this->SynchronizedWindows->SetEnabled(use_distributed_rendering);

  // Now we need a mechanism to pass this information about whether we are using
  // remote-rendering or not to the representation.

  // set the image reduction factor. 
  this->SynchronizedRenderers->SetImageReductionFactor(
    this->StillRenderImageReductionFactor);

  // Call Render() on local render window only if
  // 1: Local process is the driver OR
  // 2: RenderEventPropagation is Off and we are doing distributed rendering.
  if (this->SynchronizedWindows->GetLocalProcessIsDriver() ||
    (!this->SynchronizedWindows->GetRenderEventPropagation() && 
     use_distributed_rendering))
    {
    this->GetRenderWindow()->Render();
    }

  vtkTimerLog::MarkEndEvent("Still Render");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::InteractiveRender()
{
  //// * Decide if we are doing LOD or full-res rendering.

  //// * Decide if we are doing remote rendering or local rendering.

  //vtkTimerLog::MarkStartEvent("Interactive Render");

  //this->SynchronizedRenderers->SetImageReductionFactor(
  //  this->InteractiveRenderImageReductionFactor);

  //if (this->SynchronizedWindows->GetLocalProcessIsDriver() ||
  //  !this->SynchronizedWindows->GetRenderEventPropagation())
  //  {
  //  this->GetRenderWindow()->Render();
  //  }

  //vtkTimerLog::MarkEndEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkPVRenderView::GatherGeometrySizeInformation()
{
  this->GeometrySize = 0;
  unsigned long geometry_size = 0;
  int num_reprs = this->GetNumberOfRepresentations();
  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkDataRepresentation *drepr = this->GetRepresentation(cc);
    //vtkPVDataRepresentation *pvrepr =
    //  vtkPVDataRepresentation::SafeDownCast(drepr);
    //if (pvrepr)
    //  {
    //  geometry_size += pvrepr->GetLocalGeometrySize();
    //  }
    //else
    //  {
    //  // We can look at the input dataobject for the vtkDataRepresentation and
    //  // then get size information from it. We'll do that in future. For now,
    //  // let's assume that we don't care about such geometry size.
    //  }
    }

  // Now synchronize the geometry size.
  vtkMultiProcessController* parallelController =
    this->SynchronizedWindows->GetParallelController();
  vtkMultiProcessController* csController =
    this->SynchronizedWindows->GetClientServerController();

  if (parallelController)
    {
    unsigned long reduced_value=0;
    parallelController->AllReduce(&geometry_size, &reduced_value, 1,
      vtkCommunicator::SUM_OP);
    geometry_size = reduced_value;
    }

  if (csController)
    {
    // FIXME: won't it be easier to add some support for collective operations
    // to the vtkSocketCommunicator?
    if (this->SynchronizedWindows->GetLocalProcessIsDriver())
      {
      // client
      unsigned long value;
      csController->Broadcast(&geometry_size, 1, 0);
      csController->Broadcast(&value, 1, 1);
      geometry_size += value;
      }
    else
      {
      // server-root.
      unsigned long value;
      csController->Broadcast(&value, 1, 1);
      csController->Broadcast(&geometry_size, 1, 0);
      geometry_size += value;
      }
    }

  if (parallelController)
    {
    unsigned long reduced_value=0;
    parallelController->AllReduce(&geometry_size, &reduced_value, 1,
      vtkCommunicator::SUM_OP);
    geometry_size = reduced_value;
    }

  this->GeometrySize = geometry_size;
}

//----------------------------------------------------------------------------
bool vtkPVRenderView::GetUseDistributedRendering()
{
  return this->RemoteRenderingThreshold <= this->GeometrySize;  
}

//----------------------------------------------------------------------------
void vtkPVRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
