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

#include "vtkObjectFactory.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"
#include "vtkCommand.h"

#include "vtkAxesActor.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"

#include <assert.h>

namespace
{
  static vtkWeakPointer<vtkPVSynchronizedRenderWindows> SynchronizedWindows;
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

  this->Identifier = 0;

  // Destroy the render window superclass created.
  this->RenderWindow->RemoveObserver(this->GetObserver());
  this->RenderWindow->Delete();
  this->RenderWindow = 0;

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
  vtkSphereSource* sphere = vtkSphereSource::New();
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
  mapper->SetInputConnection(sphere->GetOutputPort());
  sphere->Delete();
  vtkActor* actor = vtkActor::New();
  actor->SetMapper(mapper);
  mapper->Delete();

  this->Renderer->AddActor(actor);
  actor->Delete();
}

//----------------------------------------------------------------------------
vtkPVRenderView::~vtkPVRenderView()
{
  this->SynchronizedWindows->RemoveAllRenderers(this->Identifier);
  this->SynchronizedWindows->RemoveRenderWindow(this->Identifier);
  this->SynchronizedWindows->Delete();
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
