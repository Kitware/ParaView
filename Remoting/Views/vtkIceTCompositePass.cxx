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
#include "vtkIceTCompositePass.h"

#include "vtkBoundingBox.h"
#include "vtkCameraPass.h"
#include "vtkFloatArray.h"
#include "vtkFrameBufferObjectBase.h"
#include "vtkHardwareSelector.h"
#include "vtkIceTContext.h"
#include "vtkIntArray.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkOrderedCompositingHelper.h"
#include "vtkPVLogger.h"
#include "vtkPixelBufferObject.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkTilesHelper.h"
#include "vtkTimerLog.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <IceT.h>
#include <IceTGL.h>
#include <assert.h>

#include "vtkCompositeZPassFS.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObjectVS.h"
#include "vtkValuePass.h"

// this helps use avoid passing ice-t types in the public API of
// vtkIceTCompositePass, thus avoiding a public dependency on IceT.
struct vtkIceTCompositePass::IceTDrawParams
{
  const IceTDouble* ProjectionMatrix;
  const IceTDouble* ModelViewMatrix;
  const IceTFloat* BackgroundColor;
  const IceTInt* ReadbackViewport;
  IceTImage Result;
};

namespace
{
static vtkIceTCompositePass* IceTDrawCallbackHandle = nullptr;
static const vtkRenderState* IceTDrawCallbackState = nullptr;

void IceTDrawCallback(const IceTDouble* projection_matrix, const IceTDouble* modelview_matrix,
  const IceTFloat* background_color, const IceTInt* readback_viewport, IceTImage result)
{
  if (IceTDrawCallbackState && IceTDrawCallbackHandle)
  {
    vtkIceTCompositePass::IceTDrawParams params;
    params.ProjectionMatrix = projection_matrix;
    params.ModelViewMatrix = modelview_matrix;
    params.BackgroundColor = background_color;
    params.ReadbackViewport = readback_viewport;
    params.Result = result;
    IceTDrawCallbackHandle->Draw(IceTDrawCallbackState, params);
  }
}

void MergeCubeAxesBounds(double bounds[6], const vtkRenderState* rState)
{
  vtkBoundingBox bbox(bounds);

  // Hande CubeAxes specifically has it wrongly implement the GetBounds()
  for (int cc = 0; cc < rState->GetPropArrayCount(); cc++)
  {
    vtkProp* prop = rState->GetPropArray()[cc];
    if (prop->GetVisibility() && prop->GetUseBounds())
    {
      if (prop->IsA("vtkGridAxes3DActor") || prop->IsA("vtkCubeAxesActor"))
      {
        vtkProp3D* prop3D = static_cast<vtkProp3D*>(prop);
        vtkBoundingBox box(prop3D->GetBounds());
        // This is the same trick used by vtkCubeAxesActor::GetRenderedBounds():
        if (box.IsValid())
        {
          box.Inflate(box.GetMaxLength());
          box.GetBounds(bounds);
          bbox.AddBounds(bounds);
        }
      }
    }
  }

  bbox.GetBounds(bounds);
}

} // end of namespace

vtkStandardNewMacro(vtkIceTCompositePass);
vtkCxxSetObjectMacro(vtkIceTCompositePass, RenderPass, vtkRenderPass);
vtkCxxSetObjectMacro(vtkIceTCompositePass, OrderedCompositingHelper, vtkOrderedCompositingHelper);
vtkCxxSetObjectMacro(vtkIceTCompositePass, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkIceTCompositePass::vtkIceTCompositePass()
  : EnableFloatValuePass(false)
  , LastRenderedDepths()
  , LastRenderedRGBA32F()
{
  this->IceTContext = vtkIceTContext::New();
  this->IceTContext->UseOpenGLOn();
  this->Controller = nullptr;
  this->RenderPass = nullptr;
  this->OrderedCompositingHelper = nullptr;
  this->TileMullions[0] = this->TileMullions[1] = 0;
  this->TileDimensions[0] = 1;
  this->TileDimensions[1] = 1;

  this->DataReplicatedOnAllProcesses = false;
  this->ImageReductionFactor = 1;

  this->RenderEmptyImages = false;
  this->UseOrderedCompositing = false;

  this->LastRenderedRGBAColors.reset(new vtkSynchronizedRenderers::vtkRawImage());

  this->PBO = nullptr;
  this->ZTexture = nullptr;
  this->Program = nullptr;

  this->DisplayRGBAResults = false;
  this->DisplayDepthResults = false;
}

//----------------------------------------------------------------------------
vtkIceTCompositePass::~vtkIceTCompositePass()
{
  if (this->PBO != nullptr)
  {
    vtkErrorMacro(<< "PixelBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->ZTexture != nullptr)
  {
    vtkErrorMacro(<< "ZTexture should have been deleted in ReleaseGraphicsResources().");
  }
  if (this->Program != nullptr)
  {
    delete this->Program;
    this->Program = nullptr;
  }

  this->SetOrderedCompositingHelper(nullptr);
  this->SetRenderPass(nullptr);
  this->SetController(nullptr);
  this->IceTContext->Delete();
  this->IceTContext = nullptr;
  this->LastRenderedRGBAColors.reset();
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::ReleaseGraphicsResources(vtkWindow* window)
{
  if (this->RenderPass != nullptr)
  {
    this->RenderPass->ReleaseGraphicsResources(window);
  }

  if (this->PBO != nullptr)
  {
    this->PBO->Delete();
    this->PBO = nullptr;
  }
  if (this->ZTexture != nullptr)
  {
    this->ZTexture->Delete();
    this->ZTexture = nullptr;
  }
  if (this->Program != nullptr)
  {
    this->Program->ReleaseGraphicsResources(window);
  }
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::SetupContext(const vtkRenderState* render_state)
{
  vtkOpenGLClearErrorMacro();

  vtkOpenGLRenderWindow* context =
    static_cast<vtkOpenGLRenderWindow*>(render_state->GetRenderer()->GetRenderWindow());
  vtkOpenGLState* ostate = context->GetState();

  // icetDiagnostics(ICET_DIAG_DEBUG | ICET_DIAG_ALL_NODES);

  // Irrespective of whether we are rendering in tile/display mode or not, we
  // need to pass appropriate tile parameters to IceT.
  this->UpdateTileInformation(render_state);

  // Set IceT compositing strategy.
  if ((this->TileDimensions[0] == 1) && (this->TileDimensions[1] == 1))
  {
    icetStrategy(ICET_STRATEGY_SEQUENTIAL);
  }
  else
  {
    icetStrategy(ICET_STRATEGY_REDUCE);
  }

  const bool use_ordered_compositing =
    (this->OrderedCompositingHelper && this->UseOrderedCompositing);

  IceTEnum const format =
    this->EnableFloatValuePass ? ICET_IMAGE_COLOR_RGBA_FLOAT : ICET_IMAGE_COLOR_RGBA_UBYTE;

  // If translucent geometry is present, then we should not include
  // ICET_DEPTH_BUFFER_BIT in  the input-buffer argument.
  if (use_ordered_compositing)
  {
    icetSetColorFormat(format);
    icetSetDepthFormat(ICET_IMAGE_DEPTH_NONE);
    icetCompositeMode(ICET_COMPOSITE_MODE_BLEND);
  }
  else
  {
    icetSetColorFormat(format);
    icetSetDepthFormat(ICET_IMAGE_DEPTH_FLOAT);
    icetDisable(ICET_COMPOSITE_ONE_BUFFER);
    icetCompositeMode(ICET_COMPOSITE_MODE_Z_BUFFER);
  }

  icetEnable(ICET_FLOATING_VIEWPORT);
  if (use_ordered_compositing)
  {
    // if ordered compositing is enabled, pass the process order from the partition ordering
    // to icet.

    // sanity check: number of rendering ranks must match number of boxes
    assert(static_cast<int>(this->OrderedCompositingHelper->GetBoundingBoxes().size()) ==
      this->IceTContext->GetController()->GetNumberOfProcesses());

    // Setup IceT context for correct sorting.
    icetEnable(ICET_ORDERED_COMPOSITE);

    // Order all the regions.
    vtkCamera* camera = render_state->GetRenderer()->GetActiveCamera();
    const auto orderedProcessIds = this->OrderedCompositingHelper->ComputeSortOrder(camera);
    if (sizeof(int) == sizeof(IceTInt))
    {
      icetCompositeOrder(reinterpret_cast<const IceTInt*>(&orderedProcessIds[0]));
    }
    else
    {
      std::vector<IceTInt> tmparray(orderedProcessIds.size());
      std::copy(orderedProcessIds.begin(), orderedProcessIds.end(), tmparray.begin());
      icetCompositeOrder(&tmparray[0]);
    }
  }
  else
  {
    icetDisable(ICET_ORDERED_COMPOSITE);
  }

  // Let IceT know the data bounds. This allows IceT to make smarter compositing
  // decisions.
  double allBounds[6];
  render_state->GetRenderer()->ComputeVisiblePropBounds(allBounds);

  // Try to detect when bounds are empty and try to let IceT know that
  // nothing is in bounds.
  if (allBounds[0] > allBounds[1])
  {
    vtkDebugMacro("nothing visible" << endl);
    IceTFloat tmp = VTK_FLOAT_MAX;
    icetBoundingVertices(1, ICET_FLOAT, 0, 1, &tmp);
  }
  else
  {
    // ComputeVisiblePropBounds() includes bounds from all props, however it
    // cannot include the "real" bounds from vtkCubeAxesActor. Thanks to
    // vtkCubeAxesActor overriding the 'GetBounds' method to return the inner
    // bounds rather that the prop  bounds for the actor. That results in BUG#
    // 13469. Hence, to overcome that issue, we iterate over the props to locate
    // vtkCubeAxesActor and include the outer bounds.
    MergeCubeAxesBounds(allBounds, render_state);

    icetBoundingBoxd(
      allBounds[0], allBounds[1], allBounds[2], allBounds[3], allBounds[4], allBounds[5]);
  }

  if (this->DataReplicatedOnAllProcesses)
  {
    icetDataReplicationGroupColor(1);
  }
  else
  {
    icetDataReplicationGroupColor(static_cast<IceTInt>(this->Controller->GetLocalProcessId()));
  }

  GLbitfield clear_mask = 0;
  if (!render_state->GetRenderer()->Transparent())
  {
    ostate->vtkglClearColor((GLclampf)(0.0), (GLclampf)(0.0), (GLclampf)(0.0), (GLclampf)(0.0));
    clear_mask |= GL_COLOR_BUFFER_BIT;
  }
  if (!render_state->GetRenderer()->GetPreserveDepthBuffer())
  {
    ostate->vtkglClearDepth(static_cast<GLclampf>(1.0));
    clear_mask |= GL_DEPTH_BUFFER_BIT;
  }
  ostate->vtkglClear(clear_mask);

  // when a painter needs to use MPI global collective
  // communications the empty images option ensures
  // that all painters including those without visible data
  // are executed
  if (this->RenderEmptyImages)
  {
    icetEnable(ICET_RENDER_EMPTY_IMAGES);
  }
  else
  {
    icetDisable(ICET_RENDER_EMPTY_IMAGES);
  }

  vtkOpenGLCheckErrorMacro("failed after SetupContext");
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::CleanupContext(const vtkRenderState*)
{
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::Render(const vtkRenderState* render_state)
{
  vtkVLogScopeF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: Render", vtkLogIdentifier(this));
  vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass::Render Start");
  this->IceTContext->SetController(this->Controller);
  if (!this->IceTContext->IsValid())
  {
    vtkErrorMacro("Could not initialize IceT context.");
    return;
  }

  vtkOpenGLRenderWindow* context =
    static_cast<vtkOpenGLRenderWindow*>(render_state->GetRenderer()->GetRenderWindow());
  vtkOpenGLState* ostate = context->GetState();

  this->IceTContext->MakeCurrent();
  this->SetupContext(render_state);

  vtkOpenGLState::ScopedglViewport vsaver(ostate);
  vtkOpenGLState::ScopedglScissor ssaver(ostate);

  ostate->vtkglViewport(0, 0, context->GetActualSize()[0], context->GetActualSize()[1]);
  ostate->vtkglScissor(0, 0, context->GetActualSize()[0], context->GetActualSize()[1]);

  GLint physical_viewport[4];
  ostate->vtkglGetIntegerv(GL_VIEWPORT, physical_viewport);
  icetPhysicalRenderSize(physical_viewport[2], physical_viewport[3]);

  icetDrawCallback(IceTDrawCallback);
  IceTDrawCallbackHandle = this;
  IceTDrawCallbackState = render_state;

  // To compute the projection matrix, we use the global aspect ratio rather
  // than the local tile's aspect which is what `vtkCamera::GetKeyMatrices`
  // does. Hence we call `this->UpdateMatrices` instead with a custom aspect
  // that fixes paraview/paraview#17611.
  IceTInt global_viewport[4];
  icetGetIntegerv(ICET_GLOBAL_VIEWPORT, global_viewport);
  this->UpdateMatrices(render_state, static_cast<double>(global_viewport[2]) / global_viewport[3]);

  float background[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

  // here is where the actual drawing occurs
  vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass: icetDrawFrame Start");
  IceTImage renderedImage =
    icetDrawFrame(this->Projection->Element[0], this->ModelView->Element[0], background);
  vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass: icetDrawFrame End");

  IceTDrawCallbackHandle = nullptr;
  IceTDrawCallbackState = nullptr;

  // isolate vtk from IceT OpenGL errors
  vtkOpenGLClearErrorMacro();

  IceTEnum const format =
    this->EnableFloatValuePass ? ICET_IMAGE_COLOR_RGBA_FLOAT : ICET_IMAGE_COLOR_RGBA_UBYTE;

  // Capture image.
  vtkIdType numPixels = icetImageGetNumPixels(renderedImage);
  if (icetImageGetColorFormat(renderedImage) != ICET_IMAGE_COLOR_NONE)
  {
    vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass: RGBA Grab Start");
    switch (format)
    {
      case ICET_IMAGE_COLOR_RGBA_FLOAT:
        // IceT requires the image format to be RGBA for float rendering
        // (R32F not supported).
        this->LastRenderedRGBA32F->SetNumberOfComponents(4);
        this->LastRenderedRGBA32F->SetNumberOfTuples(numPixels);
        icetImageCopyColorf(
          renderedImage, this->LastRenderedRGBA32F->GetPointer(0), ICET_IMAGE_COLOR_RGBA_FLOAT);
        this->LastRenderedRGBAColors->MarkInValid();
        break;

      case ICET_IMAGE_COLOR_RGBA_UBYTE:
      default:
        this->LastRenderedRGBAColors->Resize(
          icetImageGetWidth(renderedImage), icetImageGetHeight(renderedImage), 4);
        icetImageCopyColorub(renderedImage,
          this->LastRenderedRGBAColors->GetRawPtr()->GetPointer(0), ICET_IMAGE_COLOR_RGBA_UBYTE);
        this->LastRenderedRGBAColors->MarkValid();
        this->LastRenderedRGBA32F->SetNumberOfTuples(0);
        break;
    }
    vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass: RGBA Grab End");
  }
  else
  {
    this->LastRenderedRGBAColors->MarkInValid();
    this->LastRenderedRGBA32F->SetNumberOfTuples(0);
  }

  if (icetImageGetDepthFormat(renderedImage) != ICET_IMAGE_DEPTH_NONE)
  {
    vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass: Depth Grab Start");
    this->LastRenderedDepths->SetNumberOfComponents(1);
    this->LastRenderedDepths->SetNumberOfTuples(numPixels);
    icetImageCopyDepthf(
      renderedImage, this->LastRenderedDepths->GetPointer(0), ICET_IMAGE_DEPTH_FLOAT);
    vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass: Depth Grab End");
  }
  else
  {
    this->LastRenderedDepths->SetNumberOfTuples(0);
  }

  this->DisplayResultsIfNeeded(render_state);
  this->CleanupContext(render_state);

  double val = 0.;
  icetGetDoublev(ICET_COMPOSITE_TIME, &val);
  vtkTimerLog::InsertTimedEvent("ICET_COMPOSITE_TIME", val, 0);
  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "ICET_COMPOSITE_TIME: %lf", val);
  icetGetDoublev(ICET_BLEND_TIME, &val);
  vtkTimerLog::InsertTimedEvent("ICET_BLEND_TIME", val, 0);
  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "ICET_BLEND_TIME: %lf", val);
  icetGetDoublev(ICET_COMPRESS_TIME, &val);
  vtkTimerLog::InsertTimedEvent("ICET_COMPRESS_TIME", val, 0);
  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "ICET_COMPRESS_TIME: %lf", val);
  icetGetDoublev(ICET_COLLECT_TIME, &val);
  vtkTimerLog::InsertTimedEvent("ICET_COLLECT_TIME", val, 0);
  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "ICET_COLLECT_TIME: %lf", val);
  icetGetDoublev(ICET_RENDER_TIME, &val);
  vtkTimerLog::InsertTimedEvent("ICET_RENDER_TIME", val, 0);
  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "ICET_RENDER_TIME: %lf", val);
  icetGetDoublev(ICET_BUFFER_READ_TIME, &val);
  vtkTimerLog::InsertTimedEvent("ICET_BUFFER_READ_TIME", val, 0);
  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "ICET_BUFFER_READ_TIME: %lf", val);
  icetGetDoublev(ICET_BUFFER_WRITE_TIME, &val);
  vtkTimerLog::InsertTimedEvent("ICET_BUFFER_WRITE_TIME", val, 0);
  vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "ICET_BUFFER_WRITE_TIME: %lf", val);

  vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass::Render End");
}

// ----------------------------------------------------------------------------
void vtkIceTCompositePass::ReadyProgram(vtkOpenGLRenderWindow* context)
{
  assert("pre: context_exists" && context != 0);

  if (!this->Program)
  {
    this->Program = new vtkOpenGLHelper;
  }
  if (!this->Program->Program)
  {
    this->Program->Program =
      context->GetShaderCache()->ReadyShaderProgram(vtkTextureObjectVS, vtkCompositeZPassFS, "");
  }
  else
  {
    context->GetShaderCache()->ReadyShaderProgram(this->Program->Program);
  }
  if (!this->Program->Program)
  {
    vtkErrorMacro("Shader program failed to build.");
  }

  assert("post: Program_exists" && this->Program != 0);
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::Draw(
  const vtkRenderState* render_state, const vtkIceTCompositePass::IceTDrawParams& params)
{
  vtkOpenGLClearErrorMacro();

  vtkRenderer* ren = render_state->GetRenderer();
  vtkOpenGLRenderWindow* context = static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow());
  vtkOpenGLState* ostate = context->GetState();
  vtkCamera* cam = ren->GetActiveCamera();

  GLbitfield clear_mask = 0;
  ostate->vtkglClearColor((GLclampf)(0.0), (GLclampf)(0.0), (GLclampf)(0.0), (GLclampf)(0.0));

  if (!ren->Transparent())
  {
    clear_mask |= GL_COLOR_BUFFER_BIT;
  }
  if (!ren->GetPreserveDepthBuffer())
  {
    clear_mask |= GL_DEPTH_BUFFER_BIT;
  }

  double bg[3];
  ren->GetBackground(bg);
  if (clear_mask & GL_COLOR_BUFFER_BIT)
  {
    // let OSPRay know that we need black background too
    ren->SetBackground(0, 0, 0);
  }

  ostate->vtkglClear(clear_mask);
  if (this->RenderPass)
  {
    // Swap out the projection matrix for the (possibly modified) one from
    // iceT. This lets vtkCoordinate and vtkRenderer coordinate conversion
    // methods to work properly.
    vtkSmartPointer<vtkMatrix4x4> oldExplicitProj = cam->GetExplicitProjectionTransformMatrix();
    bool oldUseExplicitProj = cam->GetUseExplicitProjectionTransformMatrix();

    std::copy(
      params.ProjectionMatrix, params.ProjectionMatrix + 16, &this->IceTProjection->Element[0][0]);
    this->IceTProjection->Transpose();
    cam->SetExplicitProjectionTransformMatrix(this->IceTProjection);
    cam->UseExplicitProjectionTransformMatrixOn();

    // since icetPhysicalRenderSize is set to full window implying that we are
    // rendering to the full window, we need to update flags on the renderer
    // here so that depth peeling etc. uses correct viewport.
    // see paraview/paraview#18978
    double viewport[4];
    ren->GetViewport(viewport);
    ren->SetViewport(0, 0, 1, 1);
    this->RenderPass->Render(render_state);

    // reset viewport
    ren->SetViewport(viewport);

    // Reset the projection matrix:
    cam->SetExplicitProjectionTransformMatrix(oldExplicitProj);
    cam->SetUseExplicitProjectionTransformMatrix(oldUseExplicitProj);

    // copy the results
    if (!this->EnableFloatValuePass)
    {
      // Copy image from default buffer.
      if (icetImageGetColorFormat(params.Result) != ICET_IMAGE_COLOR_NONE)
      {
        // read in the pixels
        unsigned char* destdata = icetImageGetColorub(params.Result);
        glReadPixels(0, 0, icetImageGetWidth(params.Result), icetImageGetHeight(params.Result),
          GL_RGBA, GL_UNSIGNED_BYTE, destdata);

        // for selections we need the adjusted buffer
        // so we overwrite the RGB with the selection buffer
        vtkHardwareSelector* sel = ren->GetSelector();
        if (sel)
        {
          // copy the processed selection buffers into icet
          unsigned char* passdata = sel->GetPixelBuffer(sel->GetCurrentPass());
          if (passdata)
          {
            unsigned int* area = sel->GetArea();
            unsigned int passwidth = area[2] - area[0] + 1;
            for (int y = 0; y < icetImageGetHeight(params.Result); ++y)
            {
              for (int x = 0; x < icetImageGetWidth(params.Result); ++x)
              {
                unsigned char* pdptr = passdata + (y * passwidth + x) * 3;
                destdata[0] = pdptr[0];
                destdata[1] = pdptr[1];
                destdata[2] = pdptr[2];
                destdata += 4;
              }
            }
          }
        }
      }

      if (icetImageGetDepthFormat(params.Result) != ICET_IMAGE_DEPTH_NONE)
      {
        glReadPixels(0, 0, icetImageGetWidth(params.Result), icetImageGetHeight(params.Result),
          GL_DEPTH_COMPONENT, GL_FLOAT, icetImageGetDepthf(params.Result));
      }
    }
    else
    {
      // Copy image from the renderPass's internal buffer.
      vtkValuePass* valuePass = vtkValuePass::SafeDownCast(this->RenderPass);
      if (valuePass)
      {
        // Internal color attachment
        // IceT requires the image format to be RGBA for float rendering
        // (R32F not supported), so the entire attachment is read.
        valuePass->GetFloatImageData(GL_RGBA, icetImageGetWidth(params.Result),
          icetImageGetHeight(params.Result), icetImageGetColorf(params.Result));

        // Internal depth attachment
        valuePass->GetFloatImageData(GL_DEPTH_COMPONENT, icetImageGetWidth(params.Result),
          icetImageGetHeight(params.Result), icetImageGetDepthf(params.Result));
      }
    }
  }
  ren->SetBackground(bg[0], bg[1], bg[2]);
  vtkOpenGLCheckErrorMacro("failed after Draw");
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::UpdateTileInformation(const vtkRenderState* render_state)
{
  vtkVLogScopeF(
    PARAVIEW_LOG_RENDERING_VERBOSITY(), "%s: UpdateTileInformation", vtkLogIdentifier(this));

  const int image_reduction_factor = std::max(1, this->ImageReductionFactor);
  const int numranks = this->Controller ? this->Controller->GetNumberOfProcesses() : 1;

  auto renderer = render_state->GetRenderer();
  auto window = vtkRenderWindow::SafeDownCast(renderer->GetVTKWindow());
  assert(renderer != nullptr && window != nullptr);

  vtkVector2i tileWindowSize =
    vtkVector2i(window->GetActualSize()) / vtkVector2i(image_reduction_factor);

  vtkNew<vtkTilesHelper> helper;
  helper->SetTileDimensions(this->TileDimensions);
  helper->SetTileMullions(this->TileMullions);
  helper->SetTileWindowSize(tileWindowSize.GetData());

  icetResetTiles();
  for (int rank = 0; rank < numranks; ++rank)
  {
    int tileX, tileY;
    if (!helper->GetTileIndex(rank, &tileX, &tileY))
    {
      continue;
    }
    vtkVector2i size, origin;
    if (helper->GetTiledSizeAndOrigin(rank, size, origin, vtkVector4d(renderer->GetViewport())))
    {
      icetAddTile(origin[0], origin[1], size[0], size[1], rank);
      vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "icetAddTile(x=%d, y=%d, w=%d, h=%d, rank=%d)",
        origin[0], origin[1], size[0], size[1], rank);
    }
  }
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::GetLastRenderedTile(vtkSynchronizedRenderers::vtkRawImage& tile)
{
  tile.MarkInValid();
  if (!this->LastRenderedRGBAColors->IsValid() || this->LastRenderedRGBAColors->GetWidth() < 1 ||
    this->LastRenderedRGBAColors->GetHeight() < 1)
  {
    return;
  }

  tile = (*this->LastRenderedRGBAColors);
}

//----------------------------------------------------------------------------
vtkFloatArray* vtkIceTCompositePass::GetLastRenderedDepths()
{
  return this->LastRenderedDepths->GetNumberOfTuples() > 0 ? this->LastRenderedDepths.GetPointer()
                                                           : nullptr;
}

//----------------------------------------------------------------------------
vtkFloatArray* vtkIceTCompositePass::GetLastRenderedRGBA32F()
{
  return this->LastRenderedRGBA32F->GetNumberOfTuples() > 0 ? this->LastRenderedRGBA32F.GetPointer()
                                                            : nullptr;
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::DisplayResultsIfNeeded(const vtkRenderState* render_state)
{
  if (this->DisplayRGBAResults)
  {
    vtkSynchronizedRenderers::vtkRawImage tile;
    this->GetLastRenderedTile(tile);
    if (!tile.IsValid())
    {
      return;
    }
    vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "displaying rgba results");

    auto renderer = render_state->GetRenderer();
    vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass: display RGBA results begin");
    tile.PushToViewport(renderer);
    vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass: display RGBA results end");
    vtkOpenGLCheckErrorMacro("failed after push rgba buffer");
  }
  // tile.SaveAsPNG(std::string("/tmp/" + vtkPVLogger::GetThreadName() + ".png").c_str());

  if (this->DisplayDepthResults)
  {
    vtkVLogF(PARAVIEW_LOG_RENDERING_VERBOSITY(), "displaying depth results");
    vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass: display depth results begin");
    this->PushIceTDepthBufferToScreen(render_state);
    vtkOpenGLRenderUtilities::MarkDebugEvent("vtkIceTCompositePass: display depth results end");
    vtkOpenGLCheckErrorMacro("failed after push depth buffer");
  }
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::PushIceTDepthBufferToScreen(const vtkRenderState* render_state)
{
  vtkOpenGLClearErrorMacro();
  vtkOpenGLRenderUtilities::MarkDebugEvent(
    "Start vtkIceTCompositePass::PushIceTDepthBufferToScreen");

  // OpenGL code to copy it back
  // merly the code from vtkCompositeZPass

  // get the dimension of the buffer
  IceTInt id;
  icetGetIntegerv(ICET_TILE_DISPLAYED, &id);
  if (id < 0)
  {
    // current processes is not displaying any tile.
    return;
  }

  IceTInt ids;
  icetGetIntegerv(ICET_NUM_TILES, &ids);

  IceTInt* vp = new IceTInt[4 * ids];

  icetGetIntegerv(ICET_TILE_VIEWPORTS, vp);

  // IceTInt x=vp[4*id];
  // IceTInt y=vp[4*id+1];
  IceTInt w = vp[4 * id + 2];
  IceTInt h = vp[4 * id + 3];
  delete[] vp;

  if (this->LastRenderedDepths->GetNumberOfTuples() != w * h)
  {
    vtkErrorMacro(<< "Tile viewport size (" << w << "x" << h << ") does not"
                  << " match captured depth image ("
                  << this->LastRenderedDepths->GetNumberOfTuples() << ")");
    return;
  }

  float* depthBuffer = this->LastRenderedDepths->GetPointer(0);

  // pbo arguments.
  unsigned int dims[2];
  vtkIdType continuousInc[3];

  dims[0] = static_cast<unsigned int>(w);
  dims[1] = static_cast<unsigned int>(h);
  continuousInc[0] = 0;
  continuousInc[1] = 0;
  continuousInc[2] = 0;

  vtkOpenGLRenderWindow* context =
    vtkOpenGLRenderWindow::SafeDownCast(render_state->GetRenderer()->GetRenderWindow());
  vtkOpenGLState* ostate = context->GetState();

  if (this->PBO == nullptr)
  {
    this->PBO = vtkPixelBufferObject::New();
    this->PBO->SetContext(context);
  }
  if (this->ZTexture == nullptr)
  {
    this->ZTexture = vtkTextureObject::New();
    this->ZTexture->SetContext(context);
  }

  // client to PBO
  this->PBO->Upload2D(VTK_FLOAT, depthBuffer, dims, 1, continuousInc);

  // PBO to TO
  this->ZTexture->CreateDepth(dims[0], dims[1], vtkTextureObject::Native, this->PBO);

  // TO to FB: apply TO on quad with special zcomposite fragment shader.
  GLboolean prevColorMask[4];
  ostate->vtkglGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);
  ostate->vtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  GLboolean prevDepthTest = ostate->GetEnumState(GL_DEPTH_TEST);
  ostate->vtkglEnable(GL_DEPTH_TEST);

  GLboolean prevDepthMask;
  ostate->vtkglGetBooleanv(GL_DEPTH_WRITEMASK, &prevDepthMask);
  ostate->vtkglDepthMask(GL_TRUE);

  GLint prevDepthFunc;
  ostate->vtkglGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
  ostate->vtkglDepthFunc(GL_ALWAYS);

  this->ReadyProgram(context);

  int target_size[2], target_origin[2];
  render_state->GetRenderer()->GetTiledSizeAndOrigin(
    &target_size[0], &target_size[1], &target_origin[0], &target_origin[1]);

  this->ZTexture->Activate();
  this->Program->Program->SetUniformi("depth", this->ZTexture->GetTextureUnit());
  this->ZTexture->CopyToFrameBuffer(0, 0, w - 1, h - 1, target_origin[0], target_origin[1],
    target_origin[0] + target_size[0] - 1, target_origin[1] + target_size[1] - 1, target_size[0],
    target_size[1], this->Program->Program, this->Program->VAO);
  this->ZTexture->Deactivate();

  if (prevDepthTest)
  {
    ostate->vtkglEnable(GL_DEPTH_TEST);
  }
  else
  {
    ostate->vtkglDisable(GL_DEPTH_TEST);
  }

  ostate->vtkglDepthMask(prevDepthMask);
  ostate->vtkglDepthFunc(prevDepthFunc);
  ostate->vtkglColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);

  vtkOpenGLCheckErrorMacro("failed after PushIceTDepthBufferToScreen");
  vtkOpenGLRenderUtilities::MarkDebugEvent("End vtkIceTCompositePass::PushIceTDepthBufferToScreen");
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::UpdateMatrices(const vtkRenderState* render_state, double aspect)
{
  auto renderer = render_state->GetRenderer();
  auto camera = renderer->GetActiveCamera();
  assert(renderer != nullptr && camera != nullptr);

  if (camera->GetMTime() > this->Projection->GetMTime() ||
    renderer->GetMTime() > this->Projection->GetMTime())
  {
    this->ModelView->DeepCopy(camera->GetModelViewTransformMatrix());
    this->ModelView->Transpose();
    this->ModelView->Modified();

    this->Projection->DeepCopy(camera->GetProjectionTransformMatrix(aspect, -1, 1));
    this->Projection->Transpose();
    this->Projection->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "RenderPass: " << this->RenderPass << endl;
  os << indent << "TileDimensions: " << this->TileDimensions[0] << ", " << this->TileDimensions[1]
     << endl;
  os << indent << "TileMullions: " << this->TileMullions[0] << ", " << this->TileMullions[1]
     << endl;
  os << indent << "DataReplicatedOnAllProcesses: " << this->DataReplicatedOnAllProcesses << endl;
  os << indent << "ImageReductionFactor: " << this->ImageReductionFactor << endl;
  os << indent << "OrderedCompositingHelper: " << this->OrderedCompositingHelper << endl;
  os << indent << "UseOrderedCompositing: " << this->UseOrderedCompositing << endl;
  os << indent << "DisplayRGBAResults: " << this->DisplayRGBAResults << endl;
  os << indent << "DisplayDepthResults: " << this->DisplayDepthResults << endl;
}
