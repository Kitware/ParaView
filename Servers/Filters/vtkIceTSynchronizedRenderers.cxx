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
#include "vtkIceTSynchronizedRenderers.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkSmartPointer.h"
#include "vtkTilesHelper.h"

#include "vtkCameraPass.h"

#include <vtkgl.h>
#include <GL/ice-t.h>

namespace
{
  void IceTDrawCallback();

  class vtkMyCameraPass : public vtkCameraPass
  {
public:
  vtkTypeMacro(vtkMyCameraPass, vtkCameraPass);
  static vtkMyCameraPass* New();

  virtual void GetTiledSizeAndOrigin(
    const vtkRenderState* render_state,
    int* width, int* height, int *originX,
    int* originY)
    {
    this->Superclass::GetTiledSizeAndOrigin(render_state, width, height, originX, originY);

    *originX *= this->IceTCompositePass->GetTileDimensions()[0];
    *originY *= this->IceTCompositePass->GetTileDimensions()[1];
    *width *= this->IceTCompositePass->GetTileDimensions()[0];
    *height *= this->IceTCompositePass->GetTileDimensions()[1];
    }

  vtkIceTCompositePass* IceTCompositePass;
  };

  vtkStandardNewMacro(vtkMyCameraPass);

  class vtkInitialPass : public vtkRenderPass
  {
public:
  vtkTypeMacro(vtkInitialPass, vtkRenderPass);
  static vtkInitialPass* New();
  virtual void Render(const vtkRenderState *s)
    {
    vtkRenderer* renderer = s->GetRenderer();
    vtkRenderWindow* window = renderer->GetRenderWindow();

    // CODE COPIED FROM vtkOpenGLRenderer.
    // Oh! How I hate such kind of copying, sigh :(.
    vtkTimerLog::MarkStartEvent("vtkInitialPass::Render");

    // Do not remove this MakeCurrent! Due to Start / End methods on
    // some objects which get executed during a pipeline update,
    // other windows might get rendered since the last time
    // a MakeCurrent was called.
    window->MakeCurrent();

    // Don't do any geometry-related rendering just yet. That needs to be done
    // in the icet callback.
    this->IceTCompositePass->SetupContext(s);

    // Don't make icet render the composited image to the screen. We'll paste it
    // explicitly if needed. This is required since IceT/Viewport interactions
    // lead to weird results in multi-view configurations. Much easier to simply
    // paste back the image to the correct region after icet has rendered.
    icetDisable(ICET_DISPLAY);
    icetDisable(ICET_DISPLAY_INFLATE);
    icetDisable(ICET_CORRECT_COLORED_BACKGROUND);

    int *size = window->GetActualSize();
    glViewport(0, 0, size[0], size[1]);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 0);

    icetDrawFunc(IceTDrawCallback);
    vtkInitialPass::ActiveRenderer = renderer;
    vtkInitialPass::ActivePass = this;
    icetDrawFrame();
    vtkInitialPass::ActiveRenderer = NULL;
    vtkInitialPass::ActivePass = NULL;

    if (this->IceTSynchronizedRenderers->GetWriteBackImages())
      {
      this->IceTCompositePass->IceTInflateAndDisplay(renderer);
      }

    this->IceTCompositePass->CleanupContext(s);
    vtkTimerLog::MarkEndEvent("vtkInitialPass::Render");
    }

  static void Draw()
    {
    if (vtkInitialPass::ActiveRenderer && vtkInitialPass::ActivePass)
      {
      vtkInitialPass::ActivePass->DrawInternal(vtkInitialPass::ActiveRenderer);
      }
    }

  vtkIceTSynchronizedRenderers* IceTSynchronizedRenderers;
  vtkSetObjectMacro(IceTCompositePass, vtkIceTCompositePass);
protected:
  static vtkInitialPass* ActivePass;
  static vtkRenderer* ActiveRenderer;

  vtkInitialPass()
    {
    this->IceTCompositePass = 0;
    this->IceTSynchronizedRenderers = 0;
    }

  ~vtkInitialPass()
    {
    this->SetIceTCompositePass(0);
    this->IceTSynchronizedRenderers = 0;
    }

  void DrawInternal(vtkRenderer* ren)
    {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    this->ClearLights(ren);
    this->UpdateLightGeometry(ren);
    this->UpdateLights(ren);

    //// set matrix mode for actors
    glMatrixMode(GL_MODELVIEW);

    this->UpdateGeometry(ren);
    }

  vtkIceTCompositePass* IceTCompositePass;
  };

  vtkInitialPass* vtkInitialPass::ActivePass = NULL;
  vtkRenderer* vtkInitialPass::ActiveRenderer = NULL;
  vtkStandardNewMacro(vtkInitialPass);


  void IceTDrawCallback()
    {
    vtkInitialPass::Draw();
    }
};


vtkStandardNewMacro(vtkIceTSynchronizedRenderers);
//----------------------------------------------------------------------------
vtkIceTSynchronizedRenderers::vtkIceTSynchronizedRenderers()
{
  // First thing we do is create the ice-t render pass. This is essential since
  // most methods calls on this class simply forward it to the ice-t render
  // pass.
  this->IceTCompositePass = vtkIceTCompositePass::New();

  vtkInitialPass* initPass = vtkInitialPass::New();
  initPass->IceTSynchronizedRenderers = this;
  initPass->SetIceTCompositePass(this->IceTCompositePass);

  vtkMyCameraPass* cameraPass = vtkMyCameraPass::New();
  cameraPass->IceTCompositePass = this->IceTCompositePass;
  cameraPass->SetDelegatePass(initPass);
  initPass->Delete();

  this->RenderPass = cameraPass;
  this->SetParallelController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkIceTSynchronizedRenderers::~vtkIceTSynchronizedRenderers()
{
  this->IceTCompositePass->Delete();
  this->IceTCompositePass = 0;
  this->RenderPass->Delete();
  this->RenderPass = 0;
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::UpdateCameraAspect(int x, int y)
{
  vtkCameraPass* cp = vtkCameraPass::SafeDownCast(this->RenderPass);
  //cp->SetAspectRatioOverride((double)(x)/y);
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::RenderIceTImageToScreen()
{
  vtkRawImage &img = this->CaptureRenderedImage();
  double old_viewport[4];
  this->Renderer->GetViewport(old_viewport);

  vtkSmartPointer<vtkTilesHelper> tilesHelper = vtkSmartPointer<vtkTilesHelper>::New();
  tilesHelper->SetTileDimensions(this->IceTCompositePass->GetTileDimensions());
  tilesHelper->SetTileMullions(this->IceTCompositePass->GetTileMullions());
  tilesHelper->SetTileWindowSize(800, 800); // doesn't matter since we need
                                            // normalized viewport.

  const double* n_v = tilesHelper->GetNormalizedTileViewport(
    old_viewport, this->ParallelController->GetLocalProcessId());
  if (n_v)
    {
    this->Renderer->SetViewport(n_v[0], n_v[1], n_v[2], n_v[3]);
    img.PushToViewport(this->Renderer);
    this->Renderer->SetViewport(old_viewport);
    }
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::SetRenderer(vtkRenderer* ren)
{
  if (this->Renderer && this->Renderer->GetPass() == this->RenderPass)
    {
    this->Renderer->SetPass(NULL);
    }
  this->Superclass::SetRenderer(ren);
  if (ren)
    {
    ren->SetPass(this->RenderPass);
    }
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::SetImageReductionFactor(int val)
{
  // Don't call superclass. Since ice-t has better mechanisms for dealing with
  // image reduction factor rather than simply reducing the viewport. This
  // ensures that it works nicely in tile-display mode as well.
  // this->Superclass::SetImageReductionFactor(val);
  this->IceTCompositePass->SetImageReductionFactor(val);
}

//----------------------------------------------------------------------------
vtkSynchronizedRenderers::vtkRawImage&
vtkIceTSynchronizedRenderers::CaptureRenderedImage()
{
  // We capture the image from IceTCompositePass. This avoids the capture of
  // buffer from screen when not necessary.
  vtkRawImage& rawImage =
    (this->GetImageReductionFactor() == 1)?
    this->FullImage : this->ReducedImage;

  if (!rawImage.IsValid())
    {
    this->IceTCompositePass->GetLastRenderedTile(rawImage);
    if (!rawImage.IsValid())
      {
      vtkErrorMacro("IceT couldn't provide a tile on this process.");
      }
    }
  return rawImage;
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
