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
    vtkTimerLog::MarkStartEvent("OpenGL Dev Render");

    // Do not remove this MakeCurrent! Due to Start / End methods on
    // some objects which get executed during a pipeline update,
    // other windows might get rendered since the last time
    // a MakeCurrent was called.
    window->MakeCurrent();

    // standard render method
    this->ClearLights(renderer);
    this->UpdateCamera(renderer);

    // Don't do any geometry-related rendering just yet. That needs to be done
    // in the icet callback.
    this->IceTCompositePass->SetupContext(s);

    // FIXME: No need to display unless we are in tile-display mode.
    // icetDisable(ICET_DISPLAY);
    // icetDisable(ICET_DISPLAY_INFLATE);

    icetDrawFunc(IceTDrawCallback);
    vtkInitialPass::ActiveRenderer = renderer;
    vtkInitialPass::ActivePass = this;
    icetDrawFrame();
    vtkInitialPass::ActiveRenderer = NULL;
    vtkInitialPass::ActivePass = NULL;

    this->IceTCompositePass->CleanupContext(s);

    //// clean up the model view matrix set up by the camera
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    vtkTimerLog::MarkEndEvent("OpenGL Dev Render");
    }

  static void Draw()
    {
    if (vtkInitialPass::ActiveRenderer && vtkInitialPass::ActivePass)
      {
      vtkInitialPass::ActivePass->DrawInternal(vtkInitialPass::ActiveRenderer);
      }
    }

  vtkSetObjectMacro(IceTCompositePass, vtkIceTCompositePass);
protected:
  static vtkInitialPass* ActivePass;
  static vtkRenderer* ActiveRenderer;

  vtkInitialPass()
    {
    this->IceTCompositePass = 0;
    }

  ~vtkInitialPass()
    {
    this->SetIceTCompositePass(0);
    }

  void DrawInternal(vtkRenderer* ren)
    {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
  initPass->SetIceTCompositePass(this->IceTCompositePass);
  this->RenderPass = initPass;
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
void vtkIceTSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
