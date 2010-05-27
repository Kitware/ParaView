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
#include "vtkIceTRenderer2.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include "vtkActor.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDistributedDataFilter.h"
#include "vtkMultiProcessController.h"
#include "vtkPieceScalars.h"
#include "vtkPolyDataMapper.h"
#include "vtkSphereSource.h"

#include "vtkProcessModule.h"
#include "vtkRemoteConnection.h"

#include <assert.h>

namespace
{
  static vtkWeakPointer<vtkPVSynchronizedRenderWindows> SynchronizedWindows;

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
vtkCxxRevisionMacro(vtkPVRenderView, "$Revision$");
//----------------------------------------------------------------------------
vtkPVRenderView::vtkPVRenderView()
{
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

  this->Identifier = 0;

  // Destroy the render window superclass created.
  this->RenderWindow->RemoveObserver(this->GetObserver());
  this->RenderWindow->Delete();
  this->RenderWindow = 0;

  if (vtkProcessModule::GetProcessModule()->GetNumberOfLocalPartitions() > 1)
    {
    this->Renderer->Delete();
    this->Renderer = vtkIceTRenderer2::New();
    }

  // Get the window from the SynchronizedWindows.
  this->RenderWindow = this->SynchronizedWindows->NewRenderWindow();
  this->RenderWindow->AddRenderer(this->Renderer);

  // FIXME: This code is copied from vtkRenderView. Ideally I'd like a graceful
  // way for overriding the render window without having to duplicate code.

  if (!this->GetInteractor())
    {
    // We will handle all interactor renders by turning off rendering
    // in the interactor and listening to the interactor's render event.
    vtkSmartPointer<vtkRenderWindowInteractor> iren =
      vtkSmartPointer<vtkRenderWindowInteractor>::New();
    iren->EnableRenderOff();
    iren->AddObserver(vtkCommand::RenderEvent, this->GetObserver());
    iren->AddObserver(vtkCommand::StartInteractionEvent, this->GetObserver());
    iren->AddObserver(vtkCommand::EndInteractionEvent, this->GetObserver());
    this->RenderWindow->SetInteractor(iren);

    // The interaction mode is -1 before calling SetInteractionMode,
    // this will force an initialization of the interaction mode/style.
    this->SetInteractionModeTo3D();
    }

  // Intialize the selector and listen to render events to help Selector know when to
  // update the full-screen hardware pick.
  this->RenderWindow->AddObserver(vtkCommand::EndEvent, this->GetObserver());

  this->NonCompositedRenderer = vtkRenderer::New();
  this->NonCompositedRenderer->EraseOff();
  this->NonCompositedRenderer->InteractiveOff();
  this->NonCompositedRenderer->SetActiveCamera(this->Renderer->GetActiveCamera());
  this->RenderWindow->AddRenderer(this->NonCompositedRenderer);

  // We don't add the LabelRenderer.

  // DUMMY SPHERE FOR TESTING
  ::CreatePipeline(vtkMultiProcessController::GetGlobalController(), this->Renderer);
}

//----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView()
{
  this->SynchronizedWindows->RemoveAllRenderers(this->Identifier);
  this->SynchronizedWindows->RemoveRenderWindow(this->Identifier);
  this->SynchronizedWindows->Delete();
  this->SynchronizedRenderers->Delete();
  this->NonCompositedRenderer->Delete();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::Initialize(unsigned int id)
{
  assert(this->Identifier == 0 && id != 0);

  this->Identifier = id;
  this->SynchronizedWindows->AddRenderWindow(id, this->GetRenderWindow());
  this->SynchronizedWindows->AddRenderer(id, this->GetRenderer());
  this->SynchronizedWindows->AddRenderer(id, this->GetNonCompositedRenderer());
  this->SynchronizedRenderers->SetRenderer(this->GetRenderer());
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
  return this->GetRenderer()->GetActiveCamera();
}

//----------------------------------------------------------------------------
void vtkPVRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
