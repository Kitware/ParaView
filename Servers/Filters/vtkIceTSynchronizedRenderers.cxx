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
#include "vtkRenderPass.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"

#include "vtkgl.h"

namespace
{
  class vtkInitialPass : public vtkRenderPass
  {
public:
  vtkTypeMacro(vtkInitialPass, vtkRenderPass);
  static vtkInitialPass* New();
  virtual void Render(const vtkRenderState *s)
    {
    // This code is similar to that in vtkOpenGLRenderer::DeviceRender().

    // Do not remove this MakeCurrent! Due to Start / End methods on
    // some objects which get executed during a pipeline update,
    // other windows might get rendered since the last time
    // a MakeCurrent was called.
    s->GetRenderer()->GetRenderWindow()->MakeCurrent();

    // standard render method
    this->ClearLights(s->GetRenderer());

    this->UpdateCamera(s->GetRenderer());
    this->UpdateLightGeometry(s->GetRenderer());
    this->UpdateLights(s->GetRenderer());

    // set matrix mode for actors
    //glMatrixMode(GL_MODELVIEW);

    this->RenderPass->Render(s);

    // clean up the model view matrix set up by the camera
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    }

  vtkSetObjectMacro(RenderPass, vtkRenderPass);
protected:
  vtkInitialPass()
    {
    this->RenderPass = 0;
    }
  ~vtkInitialPass()
    {
    this->SetRenderPass(0);
    }

  vtkRenderPass* RenderPass;
  };
  vtkStandardNewMacro(vtkInitialPass);

  class vtkUpdateGeometryPass : public vtkRenderPass
  {
public:
  vtkTypeMacro(vtkUpdateGeometryPass, vtkRenderPass);
  static vtkUpdateGeometryPass* New();
  virtual void Render(const vtkRenderState *s)
    {
    // need to add code for visibility culling.
    this->UpdateGeometry(s->GetRenderer());
    }
  };
  vtkStandardNewMacro(vtkUpdateGeometryPass);
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
  initPass->SetRenderPass(this->IceTCompositePass);

  vtkUpdateGeometryPass* updateGeoPass = vtkUpdateGeometryPass::New();
  this->IceTCompositePass->SetRenderPass(updateGeoPass);
  updateGeoPass->Delete();

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
