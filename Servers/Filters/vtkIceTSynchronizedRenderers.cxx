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

#include "vtkCameraPass.h"

#include <vtkgl.h>
#include <GL/ice-t.h>

namespace
{
  void IceTDrawCallback();

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

    // Don't make icet render the composited image to the screen unless
    // necessary.
    if (this->IceTSynchronizedRenderers->GetWriteBackImages())
      {
      icetEnable(ICET_DISPLAY);
      icetEnable(ICET_DISPLAY_INFLATE);
      }
    else
      {
      icetDisable(ICET_DISPLAY);
      icetDisable(ICET_DISPLAY_INFLATE);
      }
    icetDisable(ICET_CORRECT_COLORED_BACKGROUND);

    icetDrawFunc(IceTDrawCallback);
    vtkInitialPass::ActiveRenderer = renderer;
    vtkInitialPass::ActivePass = this;
    icetDrawFrame();
    vtkInitialPass::ActiveRenderer = NULL;
    vtkInitialPass::ActivePass = NULL;

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

  vtkCameraPass* cameraPass = vtkCameraPass::New();
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
  cp->SetAspectRatioOverride((double)(x)/y);
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
