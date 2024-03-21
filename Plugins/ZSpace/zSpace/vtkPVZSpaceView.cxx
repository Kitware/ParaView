// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVZSpaceView.h"

#include "vtkCommand.h"
#include "vtkCullerCollection.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVAxesWidget.h"
#include "vtkRenderViewBase.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTransform.h"
#include "vtkZSpaceCamera.h"
#include "vtkZSpaceGenericRenderWindow.h"
#include "vtkZSpaceInteractorStyle.h"
#include "vtkZSpaceRayActor.h"
#include "vtkZSpaceRenderWindowInteractor.h"
#include "vtkZSpaceRenderer.h"
#include "vtkZSpaceSDKManager.h"

vtkStandardNewMacro(vtkPVZSpaceView);

//----------------------------------------------------------------------------
vtkPVZSpaceView::vtkPVZSpaceView()
{
  vtkZSpaceSDKManager::StereoDisplayMode mode = vtkZSpaceSDKManager::QUAD_BUFFER_STEREO;

  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  if (sdkManager)
  {
    sdkManager->InitializeZSpace();
    mode = sdkManager->GetStereoDisplayMode();
  }

  if (mode == vtkZSpaceSDKManager::STEREO_DISPLAY_API)
  {
    // Replace standard render window with dedicated one
    this->SetupZSpaceRenderWindow();
  }

  this->ZSpaceInteractorStyle->SetZSpaceRayActor(this->StylusRayActor);

  // Setup the zSpace rendering pipeline
  this->ZSpaceRenderer->AddActor(this->StylusRayActor);
  this->ZSpaceRenderer->SetActiveCamera(this->ZSpaceCamera);

  this->SetupAndSetRenderer(this->ZSpaceRenderer);
  this->ZSpaceInteractorStyle->SetCurrentRenderer(this->ZSpaceRenderer);

  // Must be done after SetRenderer.
  // Mandatory in VR-like env. to be able to see actors
  this->ZSpaceRenderer->GetCullers()->RemoveAllItems();

  // Hide axes
  this->OrientationWidget->SetParentRenderer(nullptr);

  // Override ResetCameraEvent to use the zSpace reset camera
  vtkMemberFunctionCommand<vtkPVZSpaceView>* observer =
    vtkMemberFunctionCommand<vtkPVZSpaceView>::New();

  observer->SetCallback(*this, &vtkPVZSpaceView::ResetCamera);
  this->AddObserver(vtkCommand::ResetCameraEvent, observer);
  observer->FastDelete();

  // Set the zSpace SDK Manager render window
  vtkZSpaceSDKManager::GetInstance()->SetRenderWindow(this->GetRenderWindow());
}

//----------------------------------------------------------------------------
vtkPVZSpaceView::~vtkPVZSpaceView()
{
  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  if (sdkManager)
  {
    sdkManager->ShutDown();
  }
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::SetupInteractor(vtkRenderWindowInteractor* vtkNotUsed(rwi))
{
  this->Interactor = vtkSmartPointer<vtkZSpaceRenderWindowInteractor>::New();
  this->Interactor->SetRenderWindow(this->GetRenderWindow());

  // This will set the interactor style.
  int mode = this->InteractionMode;
  this->InteractionMode = INTERACTION_MODE_UNINTIALIZED;
  this->SetInteractionMode(mode);
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::SetInteractionMode(int mode)
{
  this->Superclass::SetInteractionMode(mode);

  if (this->Interactor &&
    this->Interactor->GetInteractorStyle() != this->ZSpaceInteractorStyle.GetPointer())
  {
    switch (this->InteractionMode)
    {
      case INTERACTION_MODE_3D:
        this->Interactor->SetInteractorStyle(this->ZSpaceInteractorStyle);
        break;
      case INTERACTION_MODE_2D:
        VTK_FALLTHROUGH;
      default:
        break;
    }
  }
  this->SetActiveCamera(this->ZSpaceCamera);
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::SetStereoType(int stereoType)
{
  vtkZSpaceSDKManager::StereoDisplayMode mode = vtkZSpaceSDKManager::QUAD_BUFFER_STEREO;

  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  if (sdkManager)
  {
    mode = sdkManager->GetStereoDisplayMode();
  }

  if (mode == vtkZSpaceSDKManager::STEREO_DISPLAY_API)
  {
    // Force VTK_STEREO_ZSPACE_INSPIRE if we are on zSpace Inspire
    this->Superclass::SetStereoType(VTK_STEREO_ZSPACE_INSPIRE);
  }
  else
  {
    this->Superclass::SetStereoType(stereoType);
  }
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::ResetCamera()
{
  this->Update();

  if (this->GeometryBounds.IsValid() && !this->LockBounds && this->DiscreteCameras == nullptr)
  {
    double bounds[6];
    this->GeometryBounds.GetBounds(bounds);
    // Insure an optimal initial position of the geometry in the scene
    this->GetRenderer()->ResetCamera(bounds);
  }
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::ResetCamera(double bounds[6])
{
  if (!this->LockBounds && this->DiscreteCameras == nullptr)
  {
    // Insure an optimal initial position of the geometry in the scene
    this->GetRenderer()->ResetCamera(bounds);
  }
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::ResetAllUserTransforms()
{
  vtkActor* actor;
  vtkActorCollection* actorCollection = this->GetRenderer()->GetActors();
  vtkCollectionSimpleIterator ait;
  vtkNew<vtkTransform> identity;

  for (actorCollection->InitTraversal(ait); (actor = actorCollection->GetNextActor(ait));)
  {
    actor->SetUserTransform(identity);
  }
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::Render(bool interactive, bool skip_rendering)
{
  if (!this->GetMakingSelection())
  {
    vtkZSpaceRenderWindowInteractor::SafeDownCast(this->Interactor)->ProcessEvents();
  }

  this->Superclass::Render(interactive, skip_rendering || this->GetMakingSelection());
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PickingFieldAssociation: " << this->PickingFieldAssociation << endl;

  vtkZSpaceSDKManager::GetInstance()->PrintSelf(os, indent.GetNextIndent());
  this->ZSpaceInteractorStyle->PrintSelf(os, indent.GetNextIndent());
  this->StylusRayActor->PrintSelf(os, indent.GetNextIndent());
  this->ZSpaceCamera->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::SetInterPupillaryDistance(float interPupillaryDistance)
{
  vtkZSpaceSDKManager::GetInstance()->SetInterPupillaryDistance(interPupillaryDistance);
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::SetDrawStylus(bool drawStylus)
{
  this->StylusRayActor->SetVisibility(drawStylus);
}

//------------------------------------------------------------------------------
void vtkPVZSpaceView::SetInteractivePicking(bool interactivePicking)
{
  this->ZSpaceInteractorStyle->SetHoverPick(interactivePicking);
}

//----------------------------------------------------------------------------
void vtkPVZSpaceView::SetupZSpaceRenderWindow()
{
  // Calls related to vtkPVView::NewRenderWindow()
  vtkNew<vtkZSpaceGenericRenderWindow> window;
  window->AlphaBitPlanesOn();
  window->SetWindowName("ParaView");
  this->SetRenderWindow(window);

  // Calls related to  vtkPVView::vtkPVView()
  this->GetRenderWindow()->SetSize(this->Size);
  this->GetRenderWindow()->SetPosition(this->Position);
  this->GetRenderWindow()->SetDPI(this->PPI);

  // Calls related to vtkPVRenderView::vtkPVRenderView()
  this->GetRenderWindow()->SetMultiSamples(0);
  this->RenderView->SetRenderWindow(this->GetRenderWindow());
  this->GetRenderWindow()->AddRenderer(this->NonCompositedRenderer);
  this->GetRenderWindow()->SetNumberOfLayers(3);
}
