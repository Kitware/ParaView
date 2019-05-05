/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTSynchronizedRenderers.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIceTSynchronizedRenderers.h"

#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkCullerCollection.h"
#include "vtkImageProcessingPass.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPVDefaultPass.h"

vtkStandardNewMacro(vtkIceTSynchronizedRenderers);
//----------------------------------------------------------------------------
vtkIceTSynchronizedRenderers::vtkIceTSynchronizedRenderers()
{
  // First thing we do is create the ice-t render pass. This is essential since
  // most methods calls on this class simply forward it to the ice-t render
  // pass.
  this->IceTCompositePass = vtkIceTCompositePass::New();

  auto cameraPass = vtkCameraPass::New();
  cameraPass->SetDelegatePass(this->IceTCompositePass);
  this->CameraRenderPass = cameraPass;
  this->SetParallelController(vtkMultiProcessController::GetGlobalController());

  this->ImageProcessingPass = NULL;
  this->RenderPass = NULL;
}

//----------------------------------------------------------------------------
vtkIceTSynchronizedRenderers::~vtkIceTSynchronizedRenderers()
{
  this->IceTCompositePass->Delete();
  this->IceTCompositePass = 0;
  this->CameraRenderPass->Delete();
  this->CameraRenderPass = 0;
  this->SetImageProcessingPass(0);
  this->SetRenderPass(0);
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::SetImageProcessingPass(vtkImageProcessingPass* pass)
{
  vtkSetObjectBodyMacro(ImageProcessingPass, vtkImageProcessingPass, pass);
  if (pass && this->Renderer)
  {
    pass->SetDelegatePass(this->CameraRenderPass);
    this->Renderer->SetPass(pass);
    this->IceTCompositePass->SetDisplayRGBAResults(true);
  }
  else if (this->Renderer && this->CameraRenderPass)
  {
    this->CameraRenderPass->SetAspectRatioOverride(1.0);
    this->Renderer->SetPass(this->CameraRenderPass);
    this->IceTCompositePass->SetDisplayRGBAResults(this->WriteBackImages);
  }
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::SetUseDepthBuffer(bool useDB)
{
  this->IceTCompositePass->SetDisplayDepthResults(useDB);
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::SetRenderPass(vtkRenderPass* pass)
{
  vtkSetObjectBodyMacro(RenderPass, vtkRenderPass, pass);
  if (this->IceTCompositePass)
  {
    if (pass)
    {
      this->IceTCompositePass->SetRenderPass(pass);
    }
    else
    {
      vtkPVDefaultPass* defaultPass = vtkPVDefaultPass::New();
      this->IceTCompositePass->SetRenderPass(defaultPass);
      defaultPass->Delete();
    }
  }
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::HandleEndRender()
{
  // vtkIceTSynchronizedRenderers handles write back direcly using API on
  // vtkIceTCompositePass and hence we skip superclass' write-back
  // code.
  const bool prev = this->WriteBackImages;
  this->WriteBackImages = false;
  this->Superclass::HandleEndRender();
  this->WriteBackImages = prev;
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::SetRenderer(vtkRenderer* ren)
{
  if (this->Renderer && this->Renderer->GetPass() == this->CameraRenderPass)
  {
    this->Renderer->SetPass(NULL);
  }
  this->Superclass::SetRenderer(ren);
  if (ren)
  {
    this->Renderer->SetPass(this->CameraRenderPass);
    // icet cannot work correctly in tile-display mode is software culling is
    // applied in vtkRenderer inself. vtkIceTCompositePass will cull out-of-frustum
    // props using icet-model-view matrix later.
    ren->GetCullers()->RemoveAllItems();
  }
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::SetRenderEmptyImages(bool useREI)
{
  this->IceTCompositePass->SetRenderEmptyImages(useREI);
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
vtkSynchronizedRenderers::vtkRawImage& vtkIceTSynchronizedRenderers::CaptureRenderedImage()
{
  // We capture the image from IceTCompositePass. This avoids the capture of
  // buffer from screen when not necessary.
  vtkRawImage& rawImage =
    (this->GetImageReductionFactor() == 1) ? this->FullImage : this->ReducedImage;

  if (!rawImage.IsValid())
  {
    this->IceTCompositePass->GetLastRenderedTile(rawImage);
    if (rawImage.IsValid() && this->ImageProcessingPass)
    {
      // When using an image processing pass, we simply capture the result from
      // the active buffer. However, we do that, only when IceT produced some
      // valid result on this process.
      rawImage.Capture(this->Renderer);
    }
  }
  return rawImage;
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::SlaveStartRender()
{
  this->Superclass::SlaveStartRender();

  int x, y;
  this->IceTCompositePass->GetTileDimensions(x, y);
  if (!(x == 1 && y == 1))
  {
    // Don't mess with tile mode behavior.
    return;
  }
  // Otherwise ensure that every node starts with a black background
  // to blend onto. This is somewhat redundant, we do the same elsewhere
  // with a glClear call, but OSPRay can't see that one.
  // see also vtkPVClientServerSynchronizedRenderers
  this->Renderer->SetBackground(0, 0, 0);
  this->Renderer->SetGradientBackground(false);
  this->Renderer->SetTexturedBackground(false);
}
