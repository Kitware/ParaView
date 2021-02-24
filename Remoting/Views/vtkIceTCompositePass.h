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
/**
 * @class   vtkIceTCompositePass
 * @brief   IceT enabled render pass for distributed rendering
 *
 * vtkIceTCompositePass is a vtkRenderPass subclass that can be used for
 * compositing images (RGBA_UBYTE, depth or RGBA_32F) across processes using IceT.
 * This can be used in lieu of vtkCompositeRGBAPass. The usage of this pass
 * differs slightly from vtkCompositeRGBAPass. vtkCompositeRGBAPass composites
 * the active frame buffer, while this class requires that the render pass to
 * render info the frame buffer that needs to be composited should be set as an
 * ivar using SetRenderPass().
 *
 * This class also provides support for tile-displays. Simply set the
 * TileDimensions > [1, 1] and instead of rendering a composited image
 * on the root node, it will split the view among all tiles and generate
 * renderings on all processes.
 *
 * Warning:
 * Compositing RGBA_32F is only supported for a specific pass (vtkValuePass).
 * For a more generic integration, vtkRenderPass should expose an internal FBO
 * API.
 */

#ifndef vtkIceTCompositePass_h
#define vtkIceTCompositePass_h

#include "vtkNew.h"                 // needed for vtkWeakPointer.
#include "vtkRemotingViewsModule.h" // needed for export macro
#include "vtkRenderPass.h"
#include "vtkSynchronizedRenderers.h" //  needed for vtkRawImage.
#include <memory>                     // for std::unique_pt

class vtkFloatArray;
class vtkIceTContext;
class vtkMatrix4x4;
class vtkMultiProcessController;
class vtkOpenGLHelper;
class vtkOpenGLRenderWindow;
class vtkOrderedCompositingHelper;
class vtkPixelBufferObject;
class vtkTextureObject;
class vtkUnsignedCharArray;

class VTKREMOTINGVIEWS_EXPORT vtkIceTCompositePass : public vtkRenderPass
{
public:
  static vtkIceTCompositePass* New();
  vtkTypeMacro(vtkIceTCompositePass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  virtual void Render(const vtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  //@{
  /**
   * Controller
   * If it is nullptr, nothing will be rendered and a warning will be emitted.
   * Initial value is a nullptr pointer.
   */
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController* controller);
  //@}

  //@{
  /**
   * Get/Set the render pass used to do the actual rendering. The result of this
   * delete pass is what gets composited using IceT.
   * Initial value is a nullptr pointer.
   */
  void SetRenderPass(vtkRenderPass*);
  vtkGetObjectMacro(RenderPass, vtkRenderPass);
  //@}

  //@{
  /**
   * Get/Set the tile dimensions. Default is (1, 1). If any of the dimensions is
   * > 1 than tile display mode is assumed.
   */
  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);
  //@}

  //@{
  /**
   * Get/Set the tile mullions. The mullions are measured in pixels. Use
   * negative numbers for overlap.
   * Initial value is {0,0}.
   */
  vtkSetVector2Macro(TileMullions, int);
  vtkGetVector2Macro(TileMullions, int);
  //@}

  //@{
  /**
   * Set to true if data is replicated on all processes. This will enable IceT
   * to minimize communications since data is available on all process. Off by
   * default.
   * Initial value is false.
   */
  vtkSetMacro(DataReplicatedOnAllProcesses, bool);
  vtkGetMacro(DataReplicatedOnAllProcesses, bool);
  vtkBooleanMacro(DataReplicatedOnAllProcesses, bool);
  //@}

  //@{
  /**
   * Set the image reduction factor. This can be used to speed up compositing.
   * When using vtkIceTCompositePass use this image reduction factor rather than
   * that on vtkSynchronizedRenderers since using
   * vtkSynchronizedRenderers::ImageReductionFactor will not work correctly with
   * IceT.
   * Initial value is 1.
   */
  vtkSetClampMacro(ImageReductionFactor, int, 1, 50);
  vtkGetMacro(ImageReductionFactor, int);
  //@}

  //@{
  /**
   * partition ordering that gives processes ordering. Initial value is a nullptr pointer.
   * This is used only when UseOrderedCompositing is true.
   */
  vtkGetObjectMacro(OrderedCompositingHelper, vtkOrderedCompositingHelper);
  virtual void SetOrderedCompositingHelper(vtkOrderedCompositingHelper* helper);
  //@}

  //@{
  /**
   * Enable/disable rendering of empty images. Painters that use MPI global
   * collective communication need to enable this. Initial value is false.
   */
  vtkGetMacro(RenderEmptyImages, bool);
  vtkSetMacro(RenderEmptyImages, bool);
  vtkBooleanMacro(RenderEmptyImages, bool);
  //@}

  //@{
  /**
   * Set this to true, if compositing must be done in a specific order. This is
   * necessary when rendering volumes or translucent geometries. When
   * UseOrderedCompositing is set to true, it is expected that the
   * OrderedCompositingHelper is set as well. OrderedCompositingHelper is used
   * to decide the process-order for compositing.
   * Initial value is false.
   */
  vtkGetMacro(UseOrderedCompositing, bool);
  vtkSetMacro(UseOrderedCompositing, bool);
  vtkBooleanMacro(UseOrderedCompositing, bool);
  //@}

  /**
   * Returns the last rendered tile from this process, if any.
   * Image is invalid if tile is not available on the current process.
   */
  void GetLastRenderedTile(vtkSynchronizedRenderers::vtkRawImage& tile);

  /**
   * Provides access to the last rendered depth-buffer, if any. May return nullptr
   * if depth buffer was not composited and available on the current rank.
   */
  vtkFloatArray* GetLastRenderedDepths();

  //@{
  /**
   * Adjusts this pass to handle vtkValuePass::FLOATING_POINT, in which floating-
   * point values are rendered to vtkValuePass's internal FBO.
   */
  vtkSetMacro(EnableFloatValuePass, bool);
  //@}

  /**
   * Provides access to the last rendered float image in vtkValuePass, if any.
   * May return nullptr if a float image was not composited and is not available on
   * the current rank.
   */
  vtkFloatArray* GetLastRenderedRGBA32F();

  /**
   * Obtains the composited depth-buffer from IceT and pushes it to the screen.
   * This is only done when DepthOnly is true.
   */
  void PushIceTDepthBufferToScreen(const vtkRenderState* render_state);

  //@{
  /**
   * When set to true, vtkIceTCompositePass will push back compositing results
   * to the display on ranks where the IceT generated a composited result.
   * Generally speaking, for a vtkRenderPass, this must always be `true`.
   * However, for ParaView use cases many times we display the
   * compositing result on the server-side. In which case, we can save on the
   * extra work to push results to screen. Hence, vtkIceTCompositePass sets this
   * to false by default.
   */
  vtkSetMacro(DisplayRGBAResults, bool);
  vtkGetMacro(DisplayRGBAResults, bool);
  vtkSetMacro(DisplayDepthResults, bool);
  vtkGetMacro(DisplayDepthResults, bool);
  //@}

  //@{
  /**
   * Internal callback. Don't use.
   */
  struct IceTDrawParams;
  void Draw(const vtkRenderState* render_state, const IceTDrawParams&);
  //@}

protected:
  vtkIceTCompositePass();
  ~vtkIceTCompositePass() override;

  //@{
  /**
   * Spits the different components for the rendering process.
   */
  virtual void SetupContext(const vtkRenderState*);
  virtual void CleanupContext(const vtkRenderState*);
  //@}

  /**
   * Create program (if needed) and prepare it for texture mapping.
   * \pre context_exists: context!=0
   * \post Program_exists: this->Program!=0
   */
  void ReadyProgram(vtkOpenGLRenderWindow* context);

  /**
   * Updates the IceT tile information during each render.
   */
  void UpdateTileInformation(const vtkRenderState*);

  /**
   * Called after Icet results have been generated. vtkIceTCompositePass will
   * paste back the IceT results image to the viewport if so requested by
   * DisplayDepthResults and DisplayRGBAResults flags.
   */
  void DisplayResultsIfNeeded(const vtkRenderState*);

  /**
   * Update projection and model view matrices using current camera
   * and provided aspect ratio.
   */
  void UpdateMatrices(const vtkRenderState*, double aspect);

  vtkMultiProcessController* Controller;
  vtkOrderedCompositingHelper* OrderedCompositingHelper;
  vtkRenderPass* RenderPass;
  vtkIceTContext* IceTContext;

  bool RenderEmptyImages;
  bool UseOrderedCompositing;
  bool DataReplicatedOnAllProcesses;
  bool EnableFloatValuePass;
  int TileDimensions[2];
  int TileMullions[2];

  int ImageReductionFactor;

  bool DisplayRGBAResults;
  bool DisplayDepthResults;

  vtkNew<vtkFloatArray> LastRenderedDepths;

  vtkNew<vtkFloatArray> LastRenderedRGBA32F;

  vtkPixelBufferObject* PBO;
  vtkTextureObject* ZTexture;
  vtkOpenGLHelper* Program;

  std::unique_ptr<vtkSynchronizedRenderers::vtkRawImage> LastRenderedRGBAColors;

private:
  vtkIceTCompositePass(const vtkIceTCompositePass&) = delete;
  void operator=(const vtkIceTCompositePass&) = delete;

  vtkNew<vtkMatrix4x4> ModelView;
  vtkNew<vtkMatrix4x4> Projection;
  vtkNew<vtkMatrix4x4> IceTProjection;
};

#endif
