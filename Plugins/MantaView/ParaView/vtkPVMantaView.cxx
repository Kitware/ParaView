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
#include "vtkPVMantaView.h"

#include "vtkCamera.h"
#include "vtkDataRepresentation.h"
#include "vtkLightCollection.h"
#include "vtkMantaCamera.h"
#include "vtkMantaLight.h"
#include "vtkMantaRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkPVAxesWidget.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkRenderViewBase.h"

vtkStandardNewMacro(vtkPVMantaView);
//----------------------------------------------------------------------------
vtkPVMantaView::vtkPVMantaView()
{
  this->SynchronizedRenderers->SetDisableIceT(true);

  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::New();
  this->RenderView->SetRenderer(mantaRenderer);
  mantaRenderer->Delete();

  vtkMantaCamera* mantaCamera = vtkMantaCamera::New();
  mantaRenderer->SetActiveCamera(mantaCamera);
  mantaCamera->ParallelProjectionOff();
  mantaCamera->Delete();

  /*
    vtkMemberFunctionCommand<vtkPVRenderView>* observer =
      vtkMemberFunctionCommand<vtkPVRenderView>::New();
    observer->SetCallback(*this, &vtkPVRenderView::ResetCameraClippingRange);
    this->GetRenderer()->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,
      observer);
    observer->FastDelete();
  */
  mantaRenderer->SetUseDepthPeeling(0);

  this->Light->Delete();
  this->Light = vtkMantaLight::New();
  this->Light->SetAmbientColor(1, 1, 1);
  this->Light->SetSpecularColor(1, 1, 1);
  this->Light->SetDiffuseColor(1, 1, 1);
  this->Light->SetIntensity(1.0);
  this->Light->SetLightType(VTK_LIGHT_TYPE_HEADLIGHT);
  this->SetCurrentLight(vtkMantaLight::SafeDownCast(this->Light));

  mantaRenderer->SetAutomaticLightCreation(0);

  //  this->OrderedCompositingBSPCutsSource = vtkBSPCutsGenerator::New();

  this->OrientationWidget->SetParentRenderer(mantaRenderer);

  // this->GetRenderer()->AddActor(this->CenterAxes);

  this->SetInteractionMode(INTERACTION_MODE_3D);
}

//----------------------------------------------------------------------------
vtkPVMantaView::~vtkPVMantaView()
{
}

//----------------------------------------------------------------------------
void vtkPVMantaView::SetActiveCamera(vtkCamera* camera)
{
  this->GetRenderer()->SetActiveCamera(camera);
  //  this->GetNonCompositedRenderer()->SetActiveCamera(camera);
}

//----------------------------------------------------------------------------
void vtkPVMantaView::Initialize(unsigned int id)
{
  this->Superclass::Initialize(id);

  // disable multipass rendering so mantarenderer will do old school
  // rendering ( for OpenGL renderer )
  vtkOpenGLRenderer* glrenderer = vtkOpenGLRenderer::SafeDownCast(this->RenderView->GetRenderer());
  if (glrenderer)
  {
    glrenderer->SetPass(NULL);
  }
}

//----------------------------------------------------------------------------
void vtkPVMantaView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkPVMantaView::SetThreads(int newval)
{
  if (newval == this->Threads)
  {
    return;
  }
  this->Threads = newval;
  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(this->RenderView->GetRenderer());
  mantaRenderer->SetNumberOfWorkers(this->Threads);
}

//-----------------------------------------------------------------------------
void vtkPVMantaView::SetEnableShadows(int newval)
{
  if (newval == this->EnableShadows)
  {
    return;
  }
  this->EnableShadows = newval;
  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(this->RenderView->GetRenderer());
  mantaRenderer->SetEnableShadows(this->EnableShadows);
}

//-----------------------------------------------------------------------------
void vtkPVMantaView::SetSamples(int newval)
{
  if (newval == this->Samples)
  {
    return;
  }
  this->Samples = newval;
  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(this->RenderView->GetRenderer());
  mantaRenderer->SetSamples(this->Samples);
}

//-----------------------------------------------------------------------------
void vtkPVMantaView::SetMaxDepth(int newval)
{
  if (newval == this->EnableShadows)
  {
    return;
  }
  this->MaxDepth = newval;
  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(this->RenderView->GetRenderer());
  mantaRenderer->SetMaxDepth(this->MaxDepth);
}

//-----------------------------------------------------------------------------
void vtkPVMantaView::SetBackgroundUp(double x, double y, double z)
{
  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(this->RenderView->GetRenderer());
  mantaRenderer->SetBackgroundUp(x, y, z);
}

//-----------------------------------------------------------------------------
void vtkPVMantaView::SetBackgroundRight(double x, double y, double z)
{
  vtkMantaRenderer* mantaRenderer = vtkMantaRenderer::SafeDownCast(this->RenderView->GetRenderer());
  mantaRenderer->SetBackgroundRight(x, y, z);
}

//-----------------------------------------------------------------------------
void vtkPVMantaView::SetCurrentLight(vtkMantaLight* light)
{
  vtkLightCollection* lc = this->GetRenderer()->GetLights();
  if (!lc->IsItemPresent(light))
  {
    this->GetRenderer()->AddLight(light);
  }
}
