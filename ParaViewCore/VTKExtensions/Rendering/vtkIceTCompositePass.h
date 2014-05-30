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
// .NAME vtkIceTCompositePass - vtkRenderPass subclass for compositing
// renderings across processes using IceT.
// .SECTION Description
// vtkIceTCompositePass is a vtkRenderPass subclass that can be used for
// compositing images (rgba or depth buffer) across processes using IceT.
// This can be used in lieu of vtkCompositeRGBAPass. The usage of this pass
// differs slightly from vtkCompositeRGBAPass. vtkCompositeRGBAPass composites
// the active frame buffer, while this class requires that the render pass to
// render info the frame buffer that needs to be composited should be set as an
// ivar using SetRenderPass().
//
// This class also provides support for tile-displays. Simply set the
// TileDimensions > [1, 1] and instead of rendering a composited image
// on the root node, it will split the view among all tiles and generate
// renderings on all processes.

#ifndef __vtkIceTCompositePass_h
#define __vtkIceTCompositePass_h

#include "vtkRenderPass.h"
#include "vtkSynchronizedRenderers.h" //  needed for vtkRawImage.
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkNew.h" // needed for vtkWeakPointer.

class vtkMultiProcessController;
class vtkPKdTree;
class vtkIceTContext;
class vtkPixelBufferObject;
class vtkTextureObject;
class vtkShaderProgram2;
class vtkOpenGLRenderWindow;
class vtkUnsignedCharArray;
class vtkFloatArray;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkIceTCompositePass : public vtkRenderPass
{
public:
  static vtkIceTCompositePass* New();
  vtkTypeMacro(vtkIceTCompositePass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);
//BTX
  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
//ETX

  // Description:
  // Release graphics resources and ask components to release their own
  // resources.
  // \pre w_exists: w!=0
  void ReleaseGraphicsResources(vtkWindow *w);

  // Description:
  // Controller
  // If it is NULL, nothing will be rendered and a warning will be emitted.
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(Controller,vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController *controller);

  // Description:
  // Get/Set the render pass used to do the actual rendering. The result of this
  // delete pass is what gets composited using IceT.
  // Initial value is a NULL pointer.
  void SetRenderPass(vtkRenderPass*);
  vtkGetObjectMacro(RenderPass, vtkRenderPass);

  // Description:
  // Get/Set the tile dimensions. Default is (1, 1). If any of the dimensions is
  // > 1 than tile display mode is assumed.
  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);

  // Description:
  // Get/Set the tile mullions. The mullions are measured in pixels. Use
  // negative numbers for overlap.
  // Initial value is {0,0}.
  vtkSetVector2Macro(TileMullions, int);
  vtkGetVector2Macro(TileMullions, int);

  // Description:
  // Set to true if data is replicated on all processes. This will enable IceT
  // to minimize communications since data is available on all process. Off by
  // default.
  // Initial value is false.
  vtkSetMacro(DataReplicatedOnAllProcesses, bool);
  vtkGetMacro(DataReplicatedOnAllProcesses, bool);
  vtkBooleanMacro(DataReplicatedOnAllProcesses, bool);

  // Description:
  // Set the image reduction factor. This can be used to speed up compositing.
  // When using vtkIceTCompositePass use this image reduction factor rather than
  // that on vtkSynchronizedRenderers since using
  // vtkSynchronizedRenderers::ImageReductionFactor will not work correctly with
  // IceT.
  // Initial value is 1.
  vtkSetClampMacro(ImageReductionFactor, int, 1, 50);
  vtkGetMacro(ImageReductionFactor, int);

  // Description:
  // kd tree that gives processes ordering. Initial value is a NULL pointer.
  // This is used only when UseOrderedCompositing is true.
  vtkGetObjectMacro(KdTree,vtkPKdTree);
  virtual void SetKdTree(vtkPKdTree *kdtree);

  // Description:
  // Enable/disable rendering of empty images. Painters that use MPI global
  // collective communication need to enable this. Initial value is false.
  vtkGetMacro(RenderEmptyImages, bool);
  vtkSetMacro(RenderEmptyImages, bool);
  vtkBooleanMacro(RenderEmptyImages, bool);

  // Description:
  // Set this to true, if compositing must be done in a specific order. This is
  // necessary when rendering volumes or translucent geometries. When
  // UseOrderedCompositing is set to true, it is expected that the KdTree is set as
  // well. The KdTree is used to decide the process-order for compositing.
  // Initial value is false.
  vtkGetMacro(UseOrderedCompositing, bool);
  vtkSetMacro(UseOrderedCompositing, bool);
  vtkBooleanMacro(UseOrderedCompositing, bool);

  // Description:
  // Tell to only deal with the depth component and ignore the color
  // components.
  // If true, UseOrderedCompositing is ignored.
  // Initial value is false.
  vtkGetMacro(DepthOnly,bool);
  vtkSetMacro(DepthOnly,bool);

  // Description:
  // IceT does not deal well with the background, by setting FixBackground to
  // true, the pass will take care of displaying the correct background at the
  // price of some copy operations.
  // Initial value is false.
  vtkGetMacro(FixBackground,bool);
  vtkSetMacro(FixBackground,bool);

//BTX
  // Description:
  // Returns the last rendered tile from this process, if any.
  // Image is invalid if tile is not available on the current process.
  void GetLastRenderedTile(vtkSynchronizedRenderers::vtkRawImage& tile);

  // Description:
  // Provides access to the last rendered depth-buffer, if any. May return NULL
  // if depth buffer was not composited and available on the current rank.
  vtkFloatArray* GetLastRenderedDepths();

  // Description:
  // Obtains the composited depth-buffer from IceT and pushes it to the screen.
  // This is only done when DepthOnly is true.
  void PushIceTDepthBufferToScreen(const vtkRenderState* render_state);

  // Description:
  // Obtains the composited color-buffer from IceT and pushes it to the screen.
  // This is only done when FixBackground is true.
  void PushIceTColorBufferToScreen(const vtkRenderState* render_state);

  // Description:
  // PhysicalViewport is the viewport in the current render-window where the
  // last-rendered-tile maps.
  vtkGetVector4Macro(PhysicalViewport, double);

  // Description:
  // Internal callback. Don't use.
  virtual void Draw(const vtkRenderState*);
protected:
  vtkIceTCompositePass();
  ~vtkIceTCompositePass();

  // Description:
  // Spits the different components for the rendering process.
  virtual void SetupContext(const vtkRenderState*);
  virtual void CleanupContext(const vtkRenderState*);

  // Description:
  // Create program for texture mapping.
  // \pre context_exists: context!=0
  // \pre Program_void: this->Program==0
  // \post Program_exists: this->Program!=0
  void CreateProgram(vtkOpenGLRenderWindow *context);

  // Description:
  // Updates the IceT tile information during each render.
  void UpdateTileInformation(const vtkRenderState*);

  vtkMultiProcessController *Controller;
  vtkPKdTree *KdTree;
  vtkRenderPass* RenderPass;
  vtkIceTContext* IceTContext;

  bool RenderEmptyImages;
  bool UseOrderedCompositing;
  bool DepthOnly;
  bool DataReplicatedOnAllProcesses;
  int TileDimensions[2];
  int TileMullions[2];

  int LastTileDimensions[2];
  int LastTileMullions[2];
  int LastTileViewport[4];
  double PhysicalViewport[4];

  int ImageReductionFactor;

  vtkNew<vtkFloatArray> LastRenderedDepths;

  vtkPixelBufferObject *PBO;
  vtkTextureObject *ZTexture;
  vtkShaderProgram2 *Program;

  bool FixBackground;
  vtkTextureObject *BackgroundTexture;
  vtkTextureObject *IceTTexture;

  //Stereo Render support requires us
  //to have to raw image one for each eye so that we
  //don't overwrite the left eye with the right eyes image
  //will point at the last rendered eye
  vtkSynchronizedRenderers::vtkRawImage* LastRenderedRGBAColors;

  //actual rendered raw images for stereo. Left Eye is index 0
  //and Right Eye is index 1
  vtkSynchronizedRenderers::vtkRawImage* LastRenderedEyes[2];

private:
  vtkIceTCompositePass(const vtkIceTCompositePass&); // Not implemented
  void operator=(const vtkIceTCompositePass&); // Not implemented
//ETX
};

#endif
