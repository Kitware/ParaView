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
#include "vtkPVSynchronizedRenderer.h"

#include "vtkBoundingBox.h"
#include "vtkCameraPass.h"
#include "vtkClientServerSynchronizedRenderers.h"
#include "vtkImageProcessingPass.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVDefaultPass.h"
#include "vtkPVOptions.h"
#include "vtkPVServerInformation.h"
#include "vtkRemoteConnection.h"
#include "vtkRenderer.h"
#include "vtkSocketController.h"

#ifdef PARAVIEW_USE_ICE_T
# include "vtkIceTSynchronizedRenderers.h"
#endif

vtkStandardNewMacro(vtkPVSynchronizedRenderer);
//----------------------------------------------------------------------------
vtkPVSynchronizedRenderer::vtkPVSynchronizedRenderer()
{
  this->ImageProcessingPass = NULL;
  this->RenderPass = NULL;
  this->Enabled = true;
  this->ImageReductionFactor = 1;
  this->Renderer = 0;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro(
      "vtkPVSynchronizedRenderWindows cannot be used in the current\n"
      "setup. Aborting for debugging purposes.");
    abort();
    }

  if (pm->GetOptions()->GetProcessType() == vtkPVOptions::PVBATCH)
    {
    this->Mode = BATCH;
    }
  else if (pm->GetActiveRemoteConnection() == NULL)
    {
    this->Mode = BUILTIN;
    if (pm->GetNumberOfLocalPartitions() > 1)
      {
      this->Mode = BATCH;
      }
    }
  else if (pm->GetActiveRemoteConnection()->IsA("vtkClientConnection"))
    {
    this->Mode = SERVER;
    if (pm->GetOptions()->GetProcessType() == vtkPVOptions::PVDATA_SERVER)
      {
      this->Mode = BUILTIN;
      }
    }
  else if (pm->GetActiveRemoteConnection()->IsA("vtkServerConnection"))
    {
    this->Mode = CLIENT;
    }


  this->CSSynchronizer = 0;
  this->ParallelSynchronizer = 0;

  bool in_tile_display_mode = false;
  int tile_dims[2] = {0, 0};
  int tile_mullions[2] = {0, 0};
  vtkPVServerInformation* server_info = NULL;
  if (pm->GetActiveRemoteConnection() && this->Mode != BATCH)
    {
    vtkIdType connectionID = pm->GetConnectionID(
      pm->GetActiveRemoteConnection());
    server_info = pm->GetServerInformation(connectionID);
    }
  else
    {
    server_info = pm->GetServerInformation(0);
    }
  tile_dims[0] = server_info->GetTileDimensions()[0];
  tile_dims[1] = server_info->GetTileDimensions()[1];
  in_tile_display_mode = (tile_dims[0] > 0 || tile_dims[1] > 0);

  // we ensure that tile_dims are non-zero. We are passing the tile_dims to
  // vtkIceTSynchronizedRenderers and should be (1, 1) when not in tile-display
  // mode.
  tile_dims[0] = tile_dims[0] > 0 ? tile_dims[0] : 1;
  tile_dims[1] = tile_dims[1] > 0 ? tile_dims[1] : 1;

  tile_mullions[0] = server_info->GetTileMullions()[0];
  tile_mullions[1] = server_info->GetTileMullions()[1];

  switch (this->Mode)
    {
  case BUILTIN:
    break;

  case CLIENT:
      {
      if (in_tile_display_mode)
        {
        this->CSSynchronizer = vtkSynchronizedRenderers::New();
        this->CSSynchronizer->WriteBackImagesOff();
        }
      else
        {
        this->CSSynchronizer = vtkClientServerSynchronizedRenderers::New();
        this->CSSynchronizer->WriteBackImagesOn();
        }
      this->CSSynchronizer->SetRootProcessId(0);
      this->CSSynchronizer->SetParallelController(
        pm->GetActiveRenderServerSocketController());
      }
    break;


  case SERVER:
      {
      if (in_tile_display_mode)
        {
        this->CSSynchronizer = vtkSynchronizedRenderers::New();
        }
      else
        {
        this->CSSynchronizer = vtkClientServerSynchronizedRenderers::New();
        }
      this->CSSynchronizer->WriteBackImagesOff();
      this->CSSynchronizer->SetRootProcessId(1);
      this->CSSynchronizer->SetParallelController(
        pm->GetActiveRenderServerSocketController());
      }

    // DONT BREAK
    // break;

  case BATCH:
    if (pm->GetNumberOfLocalPartitions() > 1)
      {
#ifdef PARAVIEW_USE_ICE_T
      this->ParallelSynchronizer = vtkIceTSynchronizedRenderers::New();
      static_cast<vtkIceTSynchronizedRenderers*>(this->ParallelSynchronizer)->SetTileDimensions(
        tile_dims[0], tile_dims[1]);
      static_cast<vtkIceTSynchronizedRenderers*>(this->ParallelSynchronizer)->SetTileMullions(
        tile_mullions[0], tile_mullions[1]);
#else
      // FIXME: need to add support for compositing when not using IceT
      this->ParallelSynchronizer = vtkSynchronizedRenderers::New();
#endif
      this->ParallelSynchronizer->SetParallelController(
        vtkMultiProcessController::GetGlobalController());
      if ( (pm->GetPartitionId() == 0 && this->Mode == BATCH) ||
            in_tile_display_mode)
        {
        this->ParallelSynchronizer->WriteBackImagesOn();
        }
      else
        {
        this->ParallelSynchronizer->WriteBackImagesOff();
        }
      this->ParallelSynchronizer->SetRootProcessId(0);
      }
    break;

  default: abort();
    }

  // This ensures that CSSynchronizer simply fetches the captured buffer from
  // iceT without requiring icet to render to screen on the root node.
  if (this->ParallelSynchronizer && this->CSSynchronizer)
    {
    this->CSSynchronizer->SetCaptureDelegate(
      this->ParallelSynchronizer);
    this->ParallelSynchronizer->AutomaticEventHandlingOff();
    }
}

//----------------------------------------------------------------------------
vtkPVSynchronizedRenderer::~vtkPVSynchronizedRenderer()
{
  this->SetRenderer(0);
  if (this->ParallelSynchronizer)
    {
    this->ParallelSynchronizer->Delete();
    this->ParallelSynchronizer = 0;
    }
  if (this->CSSynchronizer)
    {
    this->CSSynchronizer->Delete();
    this->CSSynchronizer = 0;
    }
  this->SetImageProcessingPass(0);
  this->SetRenderPass(0);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetImageProcessingPass(
  vtkImageProcessingPass* pass)
{
  if (this->ImageProcessingPass == pass)
    {
    return;
    }

  vtkSetObjectBodyMacro(ImageProcessingPass, vtkImageProcessingPass, pass);
  this->SetupPasses();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetRenderPass(vtkRenderPass* pass)
{
  if (this->RenderPass == pass)
    {
    return;
    }

  vtkSetObjectBodyMacro(RenderPass, vtkRenderPass, pass);
  this->SetupPasses();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetupPasses()
{
#ifdef PARAVIEW_USE_ICE_T
  vtkIceTSynchronizedRenderers* iceTRen =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (iceTRen)
    {
    iceTRen->SetRenderPass(this->RenderPass);
    iceTRen->SetImageProcessingPass(this->ImageProcessingPass);
    return;
    }
#endif

  if (!this->Renderer)
    {
    return;
    }

  vtkCameraPass* cameraPass = vtkCameraPass::New();
  if (this->ImageProcessingPass)
    {
    this->Renderer->SetPass(this->ImageProcessingPass);
    this->ImageProcessingPass->SetDelegatePass(cameraPass);
    }
  else
    {
    this->Renderer->SetPass(cameraPass);
    }

  if (this->RenderPass)
    {
    cameraPass->SetDelegatePass(this->RenderPass);
    }
  else
    {
    vtkPVDefaultPass* defaultPass = vtkPVDefaultPass::New();
    cameraPass->SetDelegatePass(defaultPass);
    defaultPass->Delete();
    }
  cameraPass->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetEnabled(bool enabled)
{
  if (this->ParallelSynchronizer)
    {
    this->ParallelSynchronizer->SetParallelRendering(enabled);
    }
  if (this->CSSynchronizer)
    {
    this->CSSynchronizer->SetParallelRendering(enabled);
    }
  this->Enabled = enabled;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetRenderer(vtkRenderer* ren)
{
  if (this->ParallelSynchronizer)
    {
    this->ParallelSynchronizer->SetRenderer(ren);
    }
  if (this->CSSynchronizer)
    {
    this->CSSynchronizer->SetRenderer(ren);
    }
  vtkSetObjectBodyMacro(Renderer, vtkRenderer, ren);
  this->SetupPasses();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetImageReductionFactor(int factor)
{
  if (this->ImageReductionFactor == factor || factor < 1 || factor > 50)
    {
    return;
    }

  this->ImageReductionFactor = factor;

  switch (this->Mode)
    {
  case SERVER:
  case BATCH:
    if (this->ParallelSynchronizer)
      {
      this->ParallelSynchronizer->SetImageReductionFactor(this->ImageReductionFactor);
      }
    break;

  case BUILTIN:
  case CLIENT:
  default:
    break;
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetDataReplicatedOnAllProcesses(bool replicated)
{
#ifdef PARAVIEW_USE_ICE_T
  vtkIceTSynchronizedRenderers* sync =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (sync)
    {
    sync->SetDataReplicatedOnAllProcesses(replicated);
    }
#endif
  (void)replicated;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetKdTree(vtkPKdTree* tree)
{
#ifdef PARAVIEW_USE_ICE_T
  vtkIceTSynchronizedRenderers* sync =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (sync)
    {
    sync->SetKdTree(tree);
    sync->SetUseOrderedCompositing(tree != NULL);
    }
#endif
  (void)tree;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
