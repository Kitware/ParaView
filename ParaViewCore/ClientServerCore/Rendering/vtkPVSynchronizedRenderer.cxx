/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSynchronizedRenderer.cxx

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
#include "vtkCaveSynchronizedRenderers.h"
#include "vtkImageProcessingPass.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPVClientServerSynchronizedRenderers.h"
#include "vtkPVConfig.h"
#include "vtkPVDefaultPass.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderViewSettings.h"
#include "vtkPVServerInformation.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkRenderer.h"
#include "vtkSocketController.h"

#if defined PARAVIEW_USE_ICE_T && defined PARAVIEW_USE_MPI
#include "vtkIceTSynchronizedRenderers.h"
#endif
#include "vtkCompositedSynchronizedRenderers.h"

#include <assert.h>

vtkStandardNewMacro(vtkPVSynchronizedRenderer);
//----------------------------------------------------------------------------
vtkPVSynchronizedRenderer::vtkPVSynchronizedRenderer()
{
  this->ImageProcessingPass = NULL;
  this->RenderPass = NULL;
  this->Enabled = true;
  this->ImageReductionFactor = 1;
  this->Renderer = 0;
  this->UseDepthBuffer = false;
  this->Mode = INVALID;
  this->CSSynchronizer = 0;
  this->ParallelSynchronizer = 0;
  this->DisableIceT = vtkPVRenderViewSettings::GetInstance()->GetDisableIceT();
  this->DataReplicatedOnAllProcesses = false;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::Initialize(vtkPVSession* session, unsigned int id)
{
  if (this->Mode != INVALID)
  {
    vtkWarningMacro("vtkPVSynchronizedRenderer is already initialized...");
    return;
  }
  assert(this->Mode == INVALID);
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkErrorMacro("vtkPVSynchronizedRenderWindows cannot be used in the current\n"
                  "setup. Aborting for debugging purposes.");
    abort();
  }
  if (id == 0)
  {
    vtkWarningMacro("Id should not be 0.");
  }

  // active session must be a paraview-session.
  assert(session != NULL);

  int processtype = pm->GetProcessType();
  switch (processtype)
  {
    case vtkProcessModule::PROCESS_BATCH:
      this->Mode = BATCH;
      break;

    case vtkProcessModule::PROCESS_RENDER_SERVER:
    case vtkProcessModule::PROCESS_SERVER:
      this->Mode = SERVER;
      break;

    case vtkProcessModule::PROCESS_DATA_SERVER:
      this->Mode = BUILTIN;
      break;

    case vtkProcessModule::PROCESS_CLIENT:
      this->Mode = BUILTIN;
      if (session->IsA("vtkSMSessionClient"))
      {
        this->Mode = CLIENT;
      }
      break;
  }

  this->CSSynchronizer = 0;
  this->ParallelSynchronizer = 0;

  bool in_tile_display_mode = false;
  bool in_cave_mode = false;
  int tile_dims[2] = { 0, 0 };
  int tile_mullions[2] = { 0, 0 };

  vtkPVServerInformation* info = session->GetServerInformation();
  info->GetTileDimensions(tile_dims);
  in_tile_display_mode = (tile_dims[0] > 0 || tile_dims[1] > 0);
  tile_dims[0] = (tile_dims[0] == 0) ? 1 : tile_dims[0];
  tile_dims[1] = (tile_dims[1] == 0) ? 1 : tile_dims[1];
  info->GetTileMullions(tile_mullions);
  if (!in_tile_display_mode)
  {
    in_cave_mode = info->GetNumberOfMachines() > 0;
    // these are present when a pvx file is specified.
  }

  // we ensure that tile_dims are non-zero. We are passing the tile_dims to
  // vtkIceTSynchronizedRenderers and should be (1, 1) when not in tile-display
  // mode.
  tile_dims[0] = tile_dims[0] > 0 ? tile_dims[0] : 1;
  tile_dims[1] = tile_dims[1] > 0 ? tile_dims[1] : 1;

  switch (this->Mode)
  {
    case BUILTIN:
      break;

    case CLIENT:
    {
      if (in_tile_display_mode || in_cave_mode)
      {
        this->CSSynchronizer = vtkSynchronizedRenderers::New();
        this->CSSynchronizer->WriteBackImagesOff();
      }
      else
      {
        this->CSSynchronizer = vtkPVClientServerSynchronizedRenderers::New();
        this->CSSynchronizer->WriteBackImagesOn();
      }
      this->CSSynchronizer->SetRootProcessId(0);
      this->CSSynchronizer->SetParallelController(
        session->GetController(vtkPVSession::RENDER_SERVER));
    }
    break;

    case SERVER:
    {
      if (in_tile_display_mode || in_cave_mode)
      {
        this->CSSynchronizer = vtkSynchronizedRenderers::New();
      }
      else
      {
        this->CSSynchronizer = vtkPVClientServerSynchronizedRenderers::New();
      }
      this->CSSynchronizer->WriteBackImagesOff();
      this->CSSynchronizer->SetRootProcessId(1);
      this->CSSynchronizer->SetParallelController(session->GetController(vtkPVSession::CLIENT));
    }

    // DONT BREAK, server needs to setup everything in the BATCH case

    case BATCH:
      if (in_cave_mode)
      {
        this->ParallelSynchronizer = vtkCaveSynchronizedRenderers::New();
        this->ParallelSynchronizer->SetParallelController(
          vtkMultiProcessController::GetGlobalController());
        this->ParallelSynchronizer->WriteBackImagesOn();
      }
      else if (pm->GetNumberOfLocalPartitions() > 1 ||
        (pm->GetNumberOfLocalPartitions() == 1 && in_tile_display_mode))
      {
// ICET now handles stereo properly, so use it no matter the number
// of partitions
#if defined PARAVIEW_USE_ICE_T && defined PARAVIEW_USE_MPI
        if (this->DisableIceT)
        {
          this->ParallelSynchronizer = vtkCompositedSynchronizedRenderers::New();
        }
        else
        {
          vtkIceTSynchronizedRenderers* isr = vtkIceTSynchronizedRenderers::New();
          isr->SetIdentifier(id);
          isr->SetTileDimensions(tile_dims[0], tile_dims[1]);
          isr->SetTileMullions(tile_mullions[0], tile_mullions[1]);
          this->ParallelSynchronizer = isr;
        }
#else
        // FIXME: need to add support for compositing when not using IceT
        this->ParallelSynchronizer = vtkPVClientServerSynchronizedRenderers::New();
#endif
        this->ParallelSynchronizer->SetParallelController(
          vtkMultiProcessController::GetGlobalController());
        if ((pm->GetPartitionId() == 0 && this->Mode == BATCH) || in_tile_display_mode)
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

    default:
      abort();
  }

  // This ensures that CSSynchronizer simply fetches the captured buffer from
  // iceT without requiring icet to render to screen on the root node.
  if (this->ParallelSynchronizer && this->CSSynchronizer)
  {
    this->CSSynchronizer->SetCaptureDelegate(this->ParallelSynchronizer);
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
void vtkPVSynchronizedRenderer::SetLossLessCompression(bool val)
{
  vtkPVClientServerSynchronizedRenderers* cssync =
    vtkPVClientServerSynchronizedRenderers::SafeDownCast(this->CSSynchronizer);
  if (cssync)
  {
    cssync->SetLossLessCompression(val);
  }
  else
  {
    vtkDebugMacro("Not in client-server mode.");
  }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::ConfigureCompressor(const char* configuration)
{
  vtkPVClientServerSynchronizedRenderers* cssync =
    vtkPVClientServerSynchronizedRenderers::SafeDownCast(this->CSSynchronizer);
  if (cssync)
  {
    cssync->ConfigureCompressor(configuration);
  }
  else
  {
    vtkDebugMacro("Not in client-server mode.");
  }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetImageProcessingPass(vtkImageProcessingPass* pass)
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
void vtkPVSynchronizedRenderer::SetUseDepthBuffer(bool useDB)
{
  if (this->ParallelSynchronizer == 0)
  {
    return;
  }

#if defined PARAVIEW_USE_ICE_T && defined PARAVIEW_USE_MPI
  if (this->ParallelSynchronizer->IsA("vtkIceTSynchronizedRenderers") == 1)
  {
    vtkIceTSynchronizedRenderers* aux = (vtkIceTSynchronizedRenderers*)this->ParallelSynchronizer;
    aux->SetUseDepthBuffer(useDB);
  }
#else
  static_cast<void>(useDB);  // unused warning when MPI is off.
#endif
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetRenderEmptyImages(bool useREI)
{
  if (this->ParallelSynchronizer == 0)
  {
    return;
  }
#if defined PARAVIEW_USE_ICE_T && defined PARAVIEW_USE_MPI
  vtkIceTSynchronizedRenderers* sync =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (sync)
  {
    sync->SetRenderEmptyImages(useREI);
  }
#else
  static_cast<void>(useREI); // unused warning when MPI is off.
#endif
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetUseFXAA(bool enable)
{
  if (this->ParallelSynchronizer)
  {
    this->ParallelSynchronizer->SetUseFXAA(enable);
  }
  if (this->CSSynchronizer)
  {
    this->CSSynchronizer->SetUseFXAA(enable);
  }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetFXAAOptions(vtkFXAAOptions* opts)
{
  if (this->ParallelSynchronizer)
  {
    this->ParallelSynchronizer->SetFXAAOptions(opts);
  }
  if (this->CSSynchronizer)
  {
    this->CSSynchronizer->SetFXAAOptions(opts);
  }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetupPasses()
{
#if defined PARAVIEW_USE_ICE_T && defined PARAVIEW_USE_MPI
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

  // The renderer should be OpenGL ...
  vtkOpenGLRenderer* glRenderer = vtkOpenGLRenderer::SafeDownCast(ren);

  if (ren && !glRenderer)
  {
    // BUG# 13567. It's not a critical error if the renderer is not a
    // vtkOpenGLRenderer. We just don't support any render-pass stuff for such
    // renderers.
    vtkDebugMacro("Received non OpenGL renderer");
  }

  vtkSetObjectBodyMacro(Renderer, vtkOpenGLRenderer, glRenderer);
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
#if defined PARAVIEW_USE_ICE_T && defined PARAVIEW_USE_MPI
  vtkIceTSynchronizedRenderers* sync =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (sync)
  {
    sync->SetDataReplicatedOnAllProcesses(replicated);
  }
#endif
  this->DataReplicatedOnAllProcesses = replicated;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetPartitionOrdering(
  vtkPartitionOrderingInterface* partitionOrdering)
{
#if defined PARAVIEW_USE_ICE_T && defined PARAVIEW_USE_MPI
  vtkIceTSynchronizedRenderers* sync =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (sync)
  {
    sync->SetPartitionOrdering(partitionOrdering);
    sync->SetUseOrderedCompositing(partitionOrdering != NULL);
  }
#endif
  (void)partitionOrdering;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
