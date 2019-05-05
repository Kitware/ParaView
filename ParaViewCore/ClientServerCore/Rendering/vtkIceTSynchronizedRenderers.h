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
/**
 * @class   vtkIceTSynchronizedRenderers
 * @brief   vtkSynchronizedRenderers subclass that
 * uses IceT for parallel rendering and compositing.
 *
 * vtkIceTSynchronizedRenderers uses IceT (the Image Compositing Engine for
 * Tiles) for parallel rendering and compositing.
 * This class simply uses vtkIceTCompositePass internally, even though this
 * class is designed to be used with traditional renderers and not those using
 * render-passes. Note that this class internally does indeed leverage the
 * RenderPass mechanism to intercept render calls from a vtkRenderer. In other
 * words, if you are using render passes, you should not use this class. Your
 * render passes will  be overridden.
*/

#ifndef vtkIceTSynchronizedRenderers_h
#define vtkIceTSynchronizedRenderers_h

#include "vtkIceTCompositePass.h"                 // needed for inline methods.
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkSynchronizedRenderers.h"

class vtkCameraPass;
class vtkImageProcessingPass;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkIceTSynchronizedRenderers
  : public vtkSynchronizedRenderers
{
public:
  static vtkIceTSynchronizedRenderers* New();
  vtkTypeMacro(vtkIceTSynchronizedRenderers, vtkSynchronizedRenderers);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the renderer to be synchronized by this instance. A
   * vtkSynchronizedRenderers instance can be used to synchronize exactly 1
   * renderer on each processes. You can create multiple instances on
   * vtkSynchronizedRenderers to synchronize multiple renderers.
   */
  virtual void SetRenderer(vtkRenderer*) override;

  /**
   * Set the tile dimensions. Default is (1, 1).
   * If any of the dimensions is > 1 then tile display mode is assumed.
   */
  void SetTileDimensions(int x, int y) { this->IceTCompositePass->SetTileDimensions(x, y); }

  /**
   * Set the tile mullions. The mullions are measured in pixels. Use
   * negative numbers for overlap.
   */
  void SetTileMullions(int x, int y) { this->IceTCompositePass->SetTileMullions(x, y); }

  /**
   * Set to true if data is replicated on all processes. This will enable IceT
   * to minimize communications since data is available on all process. Off by
   * default.
   */
  void SetDataReplicatedOnAllProcesses(bool val)
  {
    this->IceTCompositePass->SetDataReplicatedOnAllProcesses(val);
  }

  /**
   * partition ordering that gives processes ordering. Initial value is a NULL pointer.
   * This is used only when UseOrderedCompositing is true.
   */
  void SetPartitionOrdering(vtkPartitionOrderingInterface* partitionOrdering)
  {
    this->IceTCompositePass->SetPartitionOrdering(partitionOrdering);
  }

  /**
   * Set this to true, if compositing must be done in a specific order. This is
   * necessary when rendering volumes or translucent geometries. When
   * UseOrderedCompositing is set to true, it is expected that the PartitionOrdering is set as
   * well. The PartitionOrdering is used to decide the process-order for compositing.
   */
  void SetUseOrderedCompositing(bool uoc)
  {
    this->IceTCompositePass->SetUseOrderedCompositing(uoc);
  }

  /**
   * Set the image reduction factor. Overrides superclass implementation.
   */
  virtual void SetImageReductionFactor(int val) override;
  virtual int GetImageReductionFactor() override
  {
    return this->IceTCompositePass->GetImageReductionFactor();
  }

  //@{
  /**
   * Set the parallel message communicator. This is used to communicate among
   * processes.
   */
  virtual void SetParallelController(vtkMultiProcessController* cont) override
  {
    this->Superclass::SetParallelController(cont);
    this->IceTCompositePass->SetController(cont);
  }
  //@}

  //@{
  /**
   * Get/Set an image processing pass to process the rendered images.
   */
  void SetImageProcessingPass(vtkImageProcessingPass*);
  vtkGetObjectMacro(ImageProcessingPass, vtkImageProcessingPass);
  //@}

  /**
   * Activates or de-activated the use of Depth Buffer
   */
  void SetUseDepthBuffer(bool);

  /**
   * Enable/Disable empty images optimization. Render empty images
   * is disabled by default. It may be needed if a painter needs to
   * make MPI global collective communication.
   */
  void SetRenderEmptyImages(bool);

  //@{
  /**
   * Get/Set geometry rendering pass. This pass is used to render the geometry.
   * If none specified then default rendering pipeline is used. This is
   * typically the render-pass pipeline after the CameraPass. The CameraPass is
   * setup by ParaView specially since ParaView needs some customizations for
   * multiviews/icet etc.
   */
  void SetRenderPass(vtkRenderPass*);
  vtkGetObjectMacro(RenderPass, vtkRenderPass);
  //@}

  //@{
  /**
   * Provides access to the internal vtkIceTCompositePass. Only use this if you
   * know what you're doing.
   */
  vtkGetObjectMacro(IceTCompositePass, vtkIceTCompositePass);
  //@}

protected:
  vtkIceTSynchronizedRenderers();
  ~vtkIceTSynchronizedRenderers();

  virtual void HandleEndRender() override;

  /**
   * Overridden to capture image from icet buffers instead of the screen.
   */
  virtual vtkRawImage& CaptureRenderedImage() override;

  // We use vtkIceTCompositePass internally.
  vtkCameraPass* CameraRenderPass;
  vtkIceTCompositePass* IceTCompositePass;

  // User specified custom passes
  vtkRenderPass* RenderPass;
  vtkImageProcessingPass* ImageProcessingPass;

  virtual void SlaveStartRender() override;

private:
  vtkIceTSynchronizedRenderers(const vtkIceTSynchronizedRenderers&) = delete;
  void operator=(const vtkIceTSynchronizedRenderers&) = delete;
};

#endif
