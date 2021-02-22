/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSynchronizedRenderer.cxx

  Copyright (c) Kitware, Inc.
  Copyright (c) 2017, NVIDIA CORPORATION.
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

#if VTK_MODULE_ENABLE_ParaView_icet
#include "vtkIceTSynchronizedRenderers.h"
#endif
#include "vtkCompositedSynchronizedRenderers.h"

#include <cassert>

vtkStandardNewMacro(vtkPVSynchronizedRenderer);
//----------------------------------------------------------------------------
vtkPVSynchronizedRenderer::vtkPVSynchronizedRenderer()
  : CSSynchronizer(nullptr)
  , ParallelSynchronizer(nullptr)
  , ImageProcessingPass(nullptr)
  , RenderPass(nullptr)
  , Enabled(true)
  , DisableIceT(false)
  , ImageReductionFactor(1)
  , Renderer(nullptr)
  , UseDepthBuffer(false)
  , RenderEmptyImages(false)
  , DataReplicatedOnAllProcesses(false)
  , EnableRayTracing(false)
  , EnablePathTracing(false)
  , InTileDisplayMode(false)
  , InCAVEMode(false)
{
  this->DisableIceT = vtkPVRenderViewSettings::GetInstance()->GetDisableIceT();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::Initialize(vtkPVSession* session)
{
  // session must be valid.
  assert(session != nullptr);
  assert(this->CSSynchronizer == nullptr && this->ParallelSynchronizer == nullptr);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
  {
    vtkErrorMacro("vtkPVSynchronizedRenderer cannot be used in the current\n"
                  "setup. Aborting for debugging purposes.");
    abort();
  }

  auto serverInfo = session->GetServerInformation();

  int tile_dims[2] = { 0, 0 };
  int tile_mullions[2] = { 0, 0 };
  serverInfo->GetTileDimensions(tile_dims);
  serverInfo->GetTileMullions(tile_mullions);

  this->InTileDisplayMode = (tile_dims[0] > 0 || tile_dims[1] > 0);
  this->InCAVEMode = !this->InTileDisplayMode ? (serverInfo->GetNumberOfMachines() > 0) : false;

  switch (pm->GetProcessType())
  {
    case vtkProcessModule::PROCESS_CLIENT:

      if (auto cs_controller = session->GetController(vtkPVSession::RENDER_SERVER_ROOT))
      {
        // in client-server mode.
        if (this->InTileDisplayMode || this->InCAVEMode)
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
        this->CSSynchronizer->SetParallelController(cs_controller);
      }
      else
      {
        // builtin mode, no CSSynchronizer is needed.
      }
      break;

    case vtkProcessModule::PROCESS_SERVER:
    case vtkProcessModule::PROCESS_RENDER_SERVER:
    case vtkProcessModule::PROCESS_BATCH:

      if (this->InCAVEMode)
      {
        this->ParallelSynchronizer = vtkCaveSynchronizedRenderers::New();
        this->ParallelSynchronizer->SetParallelController(pm->GetGlobalController());
        this->ParallelSynchronizer->WriteBackImagesOn();
      }
      else if (this->InTileDisplayMode || pm->GetNumberOfLocalPartitions() > 1)
      {
#if VTK_MODULE_ENABLE_ParaView_icet
        if (!this->DisableIceT)
        {
          vtkIceTSynchronizedRenderers* isr = vtkIceTSynchronizedRenderers::New();
          isr->SetTileDimensions(std::max(tile_dims[0], 1), std::max(tile_dims[1], 1));
          isr->SetTileMullions(tile_mullions[0], tile_mullions[1]);
          this->ParallelSynchronizer = isr;
        }
#endif
        if (this->ParallelSynchronizer == nullptr)
        {
          if (pm->GetNumberOfLocalPartitions() > 1)
          {
            this->ParallelSynchronizer = vtkCompositedSynchronizedRenderers::New();
          }
          else
          {
            this->ParallelSynchronizer = vtkSynchronizedRenderers::New();
          }
        }

        assert(this->ParallelSynchronizer != nullptr);

        // ensure that the server ranks always render on a black background to simplify
        // compositing and avoiding issues like #15961, #18998.
        this->ParallelSynchronizer->FixBackgroundOn();
        this->ParallelSynchronizer->SetParallelController(pm->GetGlobalController());
        this->ParallelSynchronizer->SetRootProcessId(0);
        if (this->InTileDisplayMode)
        {
          this->ParallelSynchronizer->WriteBackImagesOn();
        }
        else if (pm->GetProcessType() == vtkProcessModule::PROCESS_BATCH &&
          pm->GetPartitionId() == 0)
        {
          this->ParallelSynchronizer->WriteBackImagesOn();
        }
        else
        {
          this->ParallelSynchronizer->WriteBackImagesOff();
        }
      }

      if (auto cs_controller = session->GetController(vtkPVSession::CLIENT))
      {
        // note cs_controller will be null in batch mode and on satellites.
        assert(pm->GetPartitionId() == 0);
        if (this->InTileDisplayMode || this->InCAVEMode)
        {
          this->CSSynchronizer = vtkSynchronizedRenderers::New();
        }
        else
        {
          this->CSSynchronizer = vtkPVClientServerSynchronizedRenderers::New();
        }
        this->CSSynchronizer->WriteBackImagesOff();
        this->CSSynchronizer->SetRootProcessId(1);
        this->CSSynchronizer->SetParallelController(cs_controller);
        if (this->ParallelSynchronizer)
        {
          // This ensures that CSSynchronizer simply fetches the captured buffer from
          // iceT without requiring icet to render to screen on the root node.
          this->CSSynchronizer->SetCaptureDelegate(this->ParallelSynchronizer);
          this->ParallelSynchronizer->AutomaticEventHandlingOff();
        }
      }
      break;

    case vtkProcessModule::PROCESS_DATA_SERVER:
      break;

    default:
      vtkErrorMacro("Unknown process type detected. Aborting for debugging purposes!");
      abort();
  }

  // calling this to ensure that the `FixBackground` state on all
  // synchronized renderers is set correctly.
  this->UpdateFixBackgroundState();
}

//----------------------------------------------------------------------------
vtkPVSynchronizedRenderer::~vtkPVSynchronizedRenderer()
{
  this->SetRenderer(nullptr);
  if (this->ParallelSynchronizer)
  {
    this->ParallelSynchronizer->Delete();
    this->ParallelSynchronizer = nullptr;
  }
  if (this->CSSynchronizer)
  {
    this->CSSynchronizer->Delete();
    this->CSSynchronizer = nullptr;
  }
  this->SetImageProcessingPass(nullptr);
  this->SetRenderPass(nullptr);
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
  if (this->ParallelSynchronizer == nullptr)
  {
    return;
  }

#if VTK_MODULE_ENABLE_ParaView_icet
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
  if (this->ParallelSynchronizer == nullptr)
  {
    return;
  }
#if VTK_MODULE_ENABLE_ParaView_icet
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
void vtkPVSynchronizedRenderer::SetNVPipeSupport(bool enable)
{
  vtkPVClientServerSynchronizedRenderers* cssync =
    vtkPVClientServerSynchronizedRenderers::SafeDownCast(this->CSSynchronizer);
  if (!cssync)
  {
    return;
  }

  cssync->SetNVPipeSupport(enable);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetupPasses()
{
#if VTK_MODULE_ENABLE_ParaView_icet
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
  if (this->ParallelSynchronizer)
  {
    this->ParallelSynchronizer->SetImageReductionFactor(this->ImageReductionFactor);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetDataReplicatedOnAllProcesses(bool replicated)
{
#if VTK_MODULE_ENABLE_ParaView_icet
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
void vtkPVSynchronizedRenderer::SetOrderedCompositingHelper(vtkOrderedCompositingHelper* helper)
{
#if VTK_MODULE_ENABLE_ParaView_icet
  vtkIceTSynchronizedRenderers* sync =
    vtkIceTSynchronizedRenderers::SafeDownCast(this->ParallelSynchronizer);
  if (sync)
  {
    sync->SetOrderedCompositingHelper(helper);
    sync->SetUseOrderedCompositing(helper != nullptr);
  }
#endif
  (void)helper;
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetEnableRayTracing(bool val)
{
  if (this->EnableRayTracing != val)
  {
    this->EnableRayTracing = val;
    this->Modified();
    this->UpdateFixBackgroundState();
  }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::SetEnablePathTracing(bool val)
{
  if (this->EnablePathTracing != val)
  {
    this->EnablePathTracing = val;
    this->Modified();
    this->UpdateFixBackgroundState();
  }
}

//----------------------------------------------------------------------------
void vtkPVSynchronizedRenderer::UpdateFixBackgroundState()
{
  // note EnablePathTracing==true has no effect unless EnableRayTracing==true as well.
  if (this->CSSynchronizer)
  {
    const bool fix_background = !(this->EnableRayTracing && this->EnablePathTracing) &&
      !this->InTileDisplayMode && !this->InCAVEMode;
    this->CSSynchronizer->SetFixBackground(fix_background);
  }
  if (this->ParallelSynchronizer)
  {
    const bool fix_background = !(this->EnableRayTracing && this->EnablePathTracing);
    this->ParallelSynchronizer->SetFixBackground(fix_background);
  }
}
