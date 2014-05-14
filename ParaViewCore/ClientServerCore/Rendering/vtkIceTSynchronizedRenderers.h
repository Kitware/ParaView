/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTSynchronizedRenderers.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIceTSynchronizedRenderers - vtkSynchronizedRenderers subclass that
// uses IceT for parallel rendering and compositing.
// .SECTION Description
// vtkIceTSynchronizedRenderers uses IceT (the Image Compositing Engine for
// Tiles) for parallel rendering and compositing.
// This class simply uses vtkIceTCompositePass internally, even though this
// class is designed to be used with traditional renderers and not those using
// render-passes. Note that this class internally does indeed leverage the
// RenderPass mechanism to intercept render calls from a vtkRenderer. In other
// words, if you are using render passes, you should not use this class. Your
// render passes will  be overridden.

#ifndef __vtkIceTSynchronizedRenderers_h
#define __vtkIceTSynchronizedRenderers_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkSynchronizedRenderers.h"
#include "vtkIceTCompositePass.h" // needed for inline methods.

class vtkCameraPass;
class vtkImageProcessingPass;
class vtkMyImagePasterPass;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkIceTSynchronizedRenderers : public vtkSynchronizedRenderers
{
public:
  static vtkIceTSynchronizedRenderers* New();
  vtkTypeMacro(vtkIceTSynchronizedRenderers, vtkSynchronizedRenderers);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Identifier used to indentify the view to the vtkTileDisplayHelper.
  vtkSetMacro(Identifier, unsigned int);
  vtkGetMacro(Identifier, unsigned int);

  // Description:
  // Set the renderer to be synchronized by this instance. A
  // vtkSynchronizedRenderers instance can be used to synchronize exactly 1
  // renderer on each processes. You can create multiple instances on
  // vtkSynchronizedRenderers to synchronize multiple renderers.
  virtual void SetRenderer(vtkRenderer*);

  // Description:
  // Set the tile dimensions. Default is (1, 1).
  // If any of the dimensions is > 1 then tile display mode is assumed.
  void SetTileDimensions(int x, int y)
    { this->IceTCompositePass->SetTileDimensions(x, y); }

  // Description:
  // Set the tile mullions. The mullions are measured in pixels. Use
  // negative numbers for overlap.
  void SetTileMullions(int x, int y)
    { this->IceTCompositePass->SetTileMullions(x, y); }

  // Description:
  // Set to true if data is replicated on all processes. This will enable IceT
  // to minimize communications since data is available on all process. Off by
  // default.
  void SetDataReplicatedOnAllProcesses(bool val)
    { this->IceTCompositePass->SetDataReplicatedOnAllProcesses(val); }

  // Description:
  // kd tree that gives processes ordering. Initial value is a NULL pointer.
  // This is used only when UseOrderedCompositing is true.
  void SetKdTree(vtkPKdTree *kdtree)
    { this->IceTCompositePass->SetKdTree(kdtree); }

  // Description:
  // Set this to true, if compositing must be done in a specific order. This is
  // necessary when rendering volumes or translucent geometries. When
  // UseOrderedCompositing is set to true, it is expected that the KdTree is set as
  // well. The KdTree is used to decide the process-order for compositing.
  void SetUseOrderedCompositing(bool uoc)
    { this->IceTCompositePass->SetUseOrderedCompositing(uoc); }

  // Description:
  // Set the image reduction factor. Overrides superclass implementation.
  virtual void SetImageReductionFactor(int val);
  virtual int GetImageReductionFactor()
    { return this->IceTCompositePass->GetImageReductionFactor(); }

  // Description:
  // Set the parallel message communicator. This is used to communicate among
  // processes.
  virtual void SetParallelController(vtkMultiProcessController* cont)
    {
    this->Superclass::SetParallelController(cont);
    this->IceTCompositePass->SetController(cont);
    }

  // Description:
  // Get/Set an image processing pass to process the rendered images.
  void SetImageProcessingPass(vtkImageProcessingPass*);
  vtkGetObjectMacro(ImageProcessingPass, vtkImageProcessingPass);

  // Description:
  // Activates or de-activated the use of Depth Buffer
  void SetUseDepthBuffer(bool);

  // Description:
  // Enable/Disable empty images optimization. Render empty images
  // is disabled by default. It may be needed if a painter needs to
  // make MPI global collective communication.
  void SetRenderEmptyImages(bool);

  // Description:
  // Get/Set geometry rendering pass. This pass is used to render the geometry.
  // If none specified then default rendering pipeline is used. This is
  // typically the render-pass pipeline after the CameraPass. The CameraPass is
  // setup by ParaView specially since ParaView needs some customizations for
  // multiviews/icet etc.
  void SetRenderPass(vtkRenderPass*);
  vtkGetObjectMacro(RenderPass, vtkRenderPass);

  // Description:
  // Provides access to the internal vtkIceTCompositePass. Only use this if you
  // know what you're doing.
  vtkGetObjectMacro(IceTCompositePass, vtkIceTCompositePass);
//BTX
protected:
  vtkIceTSynchronizedRenderers();
  ~vtkIceTSynchronizedRenderers();

  unsigned int Identifier;

  virtual void HandleEndRender();

  // Description:
  // Overridden to capture image from icet buffers instead of the screen.
  virtual vtkRawImage& CaptureRenderedImage();

  // We use vtkIceTCompositePass internally.
  vtkCameraPass* CameraRenderPass;
  vtkIceTCompositePass* IceTCompositePass;
  vtkMyImagePasterPass* ImagePastingPass;

  // User specified custom passes
  vtkRenderPass* RenderPass;
  vtkImageProcessingPass *ImageProcessingPass;

private:
  vtkIceTSynchronizedRenderers(const vtkIceTSynchronizedRenderers&); // Not implemented
  void operator=(const vtkIceTSynchronizedRenderers&); // Not implemented
//ETX
};

#endif
