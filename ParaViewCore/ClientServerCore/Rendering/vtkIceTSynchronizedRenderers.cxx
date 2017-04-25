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
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPVDefaultPass.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTileDisplayHelper.h"
#include "vtkTilesHelper.h"

#include <IceTGL.h>
#include <assert.h>

// This pass is used to simply render an image onto the frame buffer. Used when
// an ImageProcessingPass is set to paste the IceT composited image into the
// frame buffer for the ImageProcessingPass.
class vtkMyImagePasterPass : public vtkRenderPass
{
public:
  static vtkMyImagePasterPass* New();
  vtkTypeMacro(vtkMyImagePasterPass, vtkRenderPass);

  vtkIceTCompositePass* IceTCompositePass;
  vtkRenderPass* DelegatePass;
  bool UseDepthBuffer;

  virtual void Render(const vtkRenderState* render_state) VTK_OVERRIDE
  {
    vtkOpenGLClearErrorMacro();
    if (this->DelegatePass)
    {
      this->DelegatePass->Render(render_state);
      vtkOpenGLCheckErrorMacro("failed after delegate pass render");
    }
    if (this->IceTCompositePass)
    {
      this->IceTCompositePass->GetLastRenderedTile(this->Image);
    }
    if (this->Image.IsValid())
    {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      this->Image.PushToFrameBuffer(render_state->GetRenderer());
      if (this->UseDepthBuffer)
      {
        this->IceTCompositePass->PushIceTDepthBufferToScreen(render_state);
      }
    }
    glFinish();
    vtkOpenGLCheckErrorMacro("failed after push depth buffer");
  }

  void SetImage(const vtkSynchronizedRenderers::vtkRawImage& image) { this->Image = image; }

  void SetUseDepthBuffer(bool useDB) { this->UseDepthBuffer = useDB; }

  virtual void ReleaseGraphicsResources(vtkWindow* window) VTK_OVERRIDE
  {
    if (this->DelegatePass)
    {
      this->DelegatePass->ReleaseGraphicsResources(window);
    }
    if (this->IceTCompositePass)
    {
      this->IceTCompositePass->ReleaseGraphicsResources(window);
    }
    this->Superclass::ReleaseGraphicsResources(window);
  }

protected:
  vtkMyImagePasterPass()
  {
    this->DelegatePass = NULL;
    this->IceTCompositePass = NULL;
    this->UseDepthBuffer = false;
  }
  ~vtkMyImagePasterPass() {}
  vtkSynchronizedRenderers::vtkRawImage Image;
};
vtkStandardNewMacro(vtkMyImagePasterPass);

namespace
{
class vtkMyCameraPass : public vtkCameraPass
{
  vtkIceTCompositePass* IceTCompositePass;

public:
  vtkTypeMacro(vtkMyCameraPass, vtkCameraPass);
  static vtkMyCameraPass* New();

  // Description:
  // Set the icet composite pass.
  vtkSetObjectMacro(IceTCompositePass, vtkIceTCompositePass);

  virtual void GetTiledSizeAndOrigin(const vtkRenderState* render_state, int* width, int* height,
    int* originX, int* originY) VTK_OVERRIDE
  {
    assert(this->IceTCompositePass != NULL);
    int tile_dims[2];
    this->IceTCompositePass->GetTileDimensions(tile_dims);
    if (tile_dims[0] > 1 || tile_dims[1] > 1)
    {
      // we have a complicated relationship with tile-scale when we are in
      // tile-display mode :).
      // vtkPVSynchronizedRenderWindows sets up the tile-scale and origin on the
      // window so that 2D annotations work just fine. However that messes up
      // when we are using IceT since IceT will do the camera translations. So
      // for IceT's sake, we reset the tile_scale/tile_viewport when doing the
      // camera transformations. Of course, this is only an issue when rendering
      // for tile-displays.
      int tile_scale[2];
      double tile_viewport[4];
      render_state->GetRenderer()->GetRenderWindow()->GetTileScale(tile_scale);
      render_state->GetRenderer()->GetRenderWindow()->GetTileViewport(tile_viewport);
      render_state->GetRenderer()->GetRenderWindow()->SetTileScale(1, 1);
      render_state->GetRenderer()->GetRenderWindow()->SetTileViewport(0, 0, 1, 1);
      this->Superclass::GetTiledSizeAndOrigin(render_state, width, height, originX, originY);
      render_state->GetRenderer()->GetRenderWindow()->SetTileScale(tile_scale);
      render_state->GetRenderer()->GetRenderWindow()->SetTileViewport(tile_viewport);

      *originX *= this->IceTCompositePass->GetTileDimensions()[0];
      *originY *= this->IceTCompositePass->GetTileDimensions()[1];
      *width *= this->IceTCompositePass->GetTileDimensions()[0];
      *height *= this->IceTCompositePass->GetTileDimensions()[1];
    }
    else
    {
      this->Superclass::GetTiledSizeAndOrigin(render_state, width, height, originX, originY);
    }
  }

  virtual void ReleaseGraphicsResources(vtkWindow* window) VTK_OVERRIDE
  {
    if (this->IceTCompositePass)
    {
      this->IceTCompositePass->ReleaseGraphicsResources(window);
    }
    this->Superclass::ReleaseGraphicsResources(window);
  }

protected:
  vtkMyCameraPass() { this->IceTCompositePass = NULL; }
  ~vtkMyCameraPass() { this->SetIceTCompositePass(0); }
};
vtkStandardNewMacro(vtkMyCameraPass);

// vtkPVIceTCompositePass extends vtkIceTCompositePass to add some ParaView
// specific rendering tweaks eg.
// * render to full viewport
// * don't let IceT paste back rendered images to the active frame buffer.
class vtkPVIceTCompositePass : public vtkIceTCompositePass
{
public:
  vtkTypeMacro(vtkPVIceTCompositePass, vtkIceTCompositePass);
  static vtkPVIceTCompositePass* New();

  // Description:
  // Updates some IceT context parameters to suit ParaView's need esp. in
  // multi-view configuration.
  virtual void SetupContext(const vtkRenderState* render_state) VTK_OVERRIDE
  {
    vtkOpenGLClearErrorMacro();
    this->Superclass::SetupContext(render_state);

    // Don't make icet render the composited image to the screen. We'll paste it
    // explicitly if needed. This is required since IceT/Viewport interactions
    // lead to weird results in multi-view configurations. Much easier to simply
    // paste back the image to the correct region after icet has rendered.
    icetDisable(ICET_GL_DISPLAY);
    icetDisable(ICET_GL_DISPLAY_INFLATE);

    if (render_state->GetFrameBuffer() == NULL)
    {
      vtkRenderWindow* window = render_state->GetRenderer()->GetRenderWindow();
      int* size = window->GetActualSize();
      glViewport(0, 0, size[0], size[1]);
    }
    glDisable(GL_SCISSOR_TEST);
    double background[3];
    render_state->GetRenderer()->GetBackground(background);
    // gradient background is not going to work correctly when using volume
    // rendering or translucent geometry, but otherwise if should work fine. We
    // set the background color so that solid background works correctly for
    // these two cases.
    glClearColor((GLclampf)background[0], (GLclampf)background[1], (GLclampf)background[2], 0.0f);
    icetEnable(ICET_CORRECT_COLORED_BACKGROUND);
    vtkOpenGLCheckErrorMacro("failed after setup context");
  }

protected:
  vtkPVIceTCompositePass()
  {
    vtkPVDefaultPass* defaultPass = vtkPVDefaultPass::New();
    this->SetRenderPass(defaultPass);
    defaultPass->Delete();
  }

  ~vtkPVIceTCompositePass() {}
};
vtkStandardNewMacro(vtkPVIceTCompositePass);
};

vtkStandardNewMacro(vtkIceTSynchronizedRenderers);
// vtkCxxSetObjectMacro(vtkIceTSynchronizedRenderers, ImageProcessingPass, vtkImageProcessingPass);
//----------------------------------------------------------------------------
vtkIceTSynchronizedRenderers::vtkIceTSynchronizedRenderers()
{
  // First thing we do is create the ice-t render pass. This is essential since
  // most methods calls on this class simply forward it to the ice-t render
  // pass.
  this->IceTCompositePass = vtkPVIceTCompositePass::New();

  vtkMyCameraPass* cameraPass = vtkMyCameraPass::New();
  cameraPass->SetDelegatePass(this->IceTCompositePass);
  cameraPass->SetIceTCompositePass(this->IceTCompositePass);
  this->CameraRenderPass = cameraPass;
  this->SetParallelController(vtkMultiProcessController::GetGlobalController());

  this->ImagePastingPass = vtkMyImagePasterPass::New();

  this->ImageProcessingPass = NULL;
  this->RenderPass = NULL;

  this->Identifier = 0;
}

//----------------------------------------------------------------------------
vtkIceTSynchronizedRenderers::~vtkIceTSynchronizedRenderers()
{
  vtkTileDisplayHelper::GetInstance()->EraseTile(this->Identifier);

  this->ImagePastingPass->Delete();
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
    int tile_dims[2];
    this->IceTCompositePass->GetTileDimensions(tile_dims);
    if (tile_dims[0] >= 1 && tile_dims[1] >= 1)
    {
      this->CameraRenderPass->SetAspectRatioOverride(tile_dims[0] * 1.0 / tile_dims[1]);
    }
    this->ImagePastingPass->DelegatePass = this->CameraRenderPass;
    this->ImagePastingPass->IceTCompositePass = this->IceTCompositePass;
    pass->SetDelegatePass(this->ImagePastingPass);
    this->Renderer->SetPass(pass);
  }
  else if (this->Renderer && this->CameraRenderPass)
  {
    this->CameraRenderPass->SetAspectRatioOverride(1.0);
    this->Renderer->SetPass(this->CameraRenderPass);
  }
}

//----------------------------------------------------------------------------
void vtkIceTSynchronizedRenderers::SetUseDepthBuffer(bool useDB)
{
  this->ImagePastingPass->SetUseDepthBuffer(useDB);
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
  if (this->WriteBackImages)
  {
    this->WriteBackImages = false;
    this->Superclass::HandleEndRender();
    this->WriteBackImages = true;
  }
  else
  {
    this->Superclass::HandleEndRender();
  }

  if (this->WriteBackImages)
  {
    vtkSynchronizedRenderers::vtkRawImage lastRenderedImage = this->CaptureRenderedImage();
    if (lastRenderedImage.IsValid())
    {
      double viewport[4];
      this->IceTCompositePass->GetPhysicalViewport(viewport);
      vtkTileDisplayHelper::GetInstance()->SetTile(
        this->Identifier, viewport, this->Renderer, lastRenderedImage);
    }
    else
    {
      vtkTileDisplayHelper::GetInstance()->EraseTile(this->Identifier);
    }

    // Write-back either the freshly rendered tile or what was most recently
    // rendered.
    vtkTileDisplayHelper::GetInstance()->FlushTiles(
      this->Identifier, this->Renderer->GetActiveCamera()->GetLeftEye());
  }
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
    // applied in vtkRenderer inself. vtkPVIceTCompositePass will cull out-of-frustum
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

#ifdef VTKGL2
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
#endif
}
