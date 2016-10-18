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
#include "vtkPVRenderViewWithShadowMap.h"

#include "vtkCameraPass.h"
#include "vtkCompositeZPass.h"
#include "vtkDepthPeelingPass.h"
#include "vtkLight.h"
#include "vtkLightsPass.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOpaquePass.h"
#include "vtkOverlayPass.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkRenderPassCollection.h"
#include "vtkSequencePass.h"
#include "vtkShadowMapBakerPass.h"
#include "vtkShadowMapPass.h"
#include "vtkTranslucentPass.h"
#include "vtkVolumetricPass.h"

namespace
{
vtkRenderPass* CreateShadowMapPipeline()
{
  vtkOpaquePass* opaque = vtkOpaquePass::New();
  vtkDepthPeelingPass* peeling = vtkDepthPeelingPass::New();
  peeling->SetMaximumNumberOfPeels(200);
  peeling->SetOcclusionRatio(0.1);
  vtkTranslucentPass* translucent = vtkTranslucentPass::New();
  peeling->SetTranslucentPass(translucent);
  vtkVolumetricPass* volume = vtkVolumetricPass::New();
  vtkOverlayPass* overlay = vtkOverlayPass::New();
  vtkLightsPass* lights = vtkLightsPass::New();

  vtkSequencePass* opaqueSequence = vtkSequencePass::New();

  vtkRenderPassCollection* passes2 = vtkRenderPassCollection::New();
  passes2->AddItem(lights);
  passes2->AddItem(opaque);
  opaqueSequence->SetPasses(passes2);
  passes2->Delete();

  vtkCameraPass* opaqueCameraPass = vtkCameraPass::New();
  opaqueCameraPass->SetDelegatePass(opaqueSequence);

  vtkShadowMapBakerPass* shadowsBaker = vtkShadowMapBakerPass::New();
  shadowsBaker->SetOpaquePass(opaqueCameraPass);
  opaqueCameraPass->Delete();
  shadowsBaker->SetResolution(256);
  // To cancel self-shadowing.
  shadowsBaker->SetPolygonOffsetFactor(3.1f);
  shadowsBaker->SetPolygonOffsetUnits(10.0f);

  vtkShadowMapPass* shadows = vtkShadowMapPass::New();
  shadows->SetShadowMapBakerPass(shadowsBaker);
  shadows->SetOpaquePass(opaqueSequence);

  if (vtkMultiProcessController::GetGlobalController())
  {
    vtkCompositeZPass* compositeZPass = vtkCompositeZPass::New();
    compositeZPass->SetController(vtkMultiProcessController::GetGlobalController());
    shadowsBaker->SetCompositeZPass(compositeZPass);
    compositeZPass->Delete();
  }

  vtkSequencePass* seq = vtkSequencePass::New();
  vtkRenderPassCollection* passes = vtkRenderPassCollection::New();
  passes->AddItem(shadowsBaker);
  passes->AddItem(shadows);
  passes->AddItem(lights);
  passes->AddItem(peeling);
  passes->AddItem(volume);
  passes->AddItem(overlay);
  seq->SetPasses(passes);

  opaque->Delete();
  peeling->Delete();
  translucent->Delete();
  volume->Delete();
  overlay->Delete();
  passes->Delete();
  lights->Delete();
  shadows->Delete();
  shadowsBaker->Delete();
  opaqueSequence->Delete();
  return seq;
}
}

vtkStandardNewMacro(vtkPVRenderViewWithShadowMap);
//----------------------------------------------------------------------------
vtkPVRenderViewWithShadowMap::vtkPVRenderViewWithShadowMap()
{
}

//----------------------------------------------------------------------------
vtkPVRenderViewWithShadowMap::~vtkPVRenderViewWithShadowMap()
{
}

//----------------------------------------------------------------------------
void vtkPVRenderViewWithShadowMap::Initialize(unsigned int id)
{
  this->Superclass::Initialize(id);

  vtkRenderPass* shadowMapPass = CreateShadowMapPipeline();
  this->SynchronizedRenderers->SetRenderPass(shadowMapPass);
  shadowMapPass->Delete();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewWithShadowMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
