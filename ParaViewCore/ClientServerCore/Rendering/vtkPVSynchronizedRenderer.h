/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSynchronizedRenderer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVSynchronizedRenderer
 * @brief coordinates rendering between corresponding renderers across multiple
 *        processes
 *
 * vtkPVSynchronizedRenderer coordinates rendering between renderers that are
 * split across multiple ranks. It internally uses other
 * vtkSynchronizedRenderers subclasses based on the operation mode to do the
 * actual coordination such as vtkCaveSynchronizedRenderers,
 * vtkIceTSynchronizedRenderers, vtkPVClientServerSynchronizedRenderers and
 * vtkCompositedSynchronizedRenderers.
 *
 */

#ifndef vtkPVSynchronizedRenderer_h
#define vtkPVSynchronizedRenderer_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class vtkIceTSynchronizedRenderers;
class vtkImageProcessingPass;
class vtkPartitionOrderingInterface;
class vtkPVSession;
class vtkRenderer;
class vtkRenderPass;
class vtkSynchronizedRenderers;
class vtkOpenGLRenderer;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVSynchronizedRenderer : public vtkObject
{
public:
  static vtkPVSynchronizedRenderer* New();
  vtkTypeMacro(vtkPVSynchronizedRenderer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set this flag to true before calling Initialize() to disable using
   * vtkIceTSynchronizedRenderers for parallel rendering.
   * Default value is setup using vtkPVRenderViewSettings::GetDisableIceT.
   */
  vtkSetMacro(DisableIceT, bool);
  vtkGetMacro(DisableIceT, bool);
  //@}

  /**
   * Must be called once to initialize the instance. This will create
   * appropriate internal vtkSynchronizedRenderers subclasses based on the
   * process type and session provided.
   */
  void Initialize(vtkPVSession* session);

  /**
   * partition ordering that gives processes ordering. Initial value is a NULL pointer.
   * This is used only when UseOrderedCompositing is true.
   */
  void SetPartitionOrdering(vtkPartitionOrderingInterface* partitionOrdering);

  /**
   * Set the renderer that is being synchronized.
   */
  void SetRenderer(vtkRenderer*);

  //@{
  /**
   * Enable/Disable parallel rendering.
   */
  virtual void SetEnabled(bool enabled);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);
  //@}

  //@{
  /**
   * Get/Set the image reduction factor.
   * This needs to be set on all processes and must match up.
   */
  void SetImageReductionFactor(int);
  vtkGetMacro(ImageReductionFactor, int);
  //@}

  //@{
  /**
   * Set to true if data is replicated on all processes. This will enable IceT
   * to minimize communications since data is available on all process. Off by
   * default.
   */
  void SetDataReplicatedOnAllProcesses(bool);
  vtkGetMacro(DataReplicatedOnAllProcesses, bool);
  vtkBooleanMacro(DataReplicatedOnAllProcesses, bool);
  //@}

  //@{
  /**
   * Get/Set an image processing pass to process the rendered images.
   */
  void SetImageProcessingPass(vtkImageProcessingPass*);
  vtkGetObjectMacro(ImageProcessingPass, vtkImageProcessingPass);
  //@}

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
   * Passes the compressor configuration to the client-server synchronizer, if
   * any. This affects the image compression used to relay images back to the
   * client.
   * See vtkPVClientServerSynchronizedRenderers::ConfigureCompressor() for
   * details.
   */
  void ConfigureCompressor(const char* configuration);
  void SetLossLessCompression(bool);
  //@}

  /**
   * Activates or de-activated the use of Depth Buffer in an ImageProcessingPass
   */
  void SetUseDepthBuffer(bool);

  /**
   * Enable/Disable empty images optimization.
   */
  void SetRenderEmptyImages(bool);

  /**
   * Enable/Disable NVPipe
   */
  void SetNVPipeSupport(bool);

  //@{
  /**
   * Not for the faint hearted. This internal vtkSynchronizedRenderers instances
   * are exposed for advanced users that want to do advanced tricks with
   * rendering. These will change without notice. Do not use them unless you
   * know what you are doing.
   * ParallelSynchronizer is the vtkSynchronizedRenderers used to synchronize
   * rendering between processes in an MPI group -- typically
   * vtkIceTSynchronizedRenderers when available.
   * CSSynchronizer is the client-server vtkSynchronizedRenderers used in
   * client-server configurations.
   */
  vtkGetObjectMacro(ParallelSynchronizer, vtkSynchronizedRenderers);
  vtkGetObjectMacro(CSSynchronizer, vtkSynchronizedRenderers);
  //@}

protected:
  vtkPVSynchronizedRenderer();
  ~vtkPVSynchronizedRenderer() override;

  /**
   * Sets up the render passes on the renderer. This won't get called on
   * processes where vtkIceTSynchronizedRenderers is used. In that case the
   * passes are forwarded to the vtkIceTSynchronizedRenderers instance.
   */
  virtual void SetupPasses();

  vtkSynchronizedRenderers* CSSynchronizer;
  vtkSynchronizedRenderers* ParallelSynchronizer;
  vtkImageProcessingPass* ImageProcessingPass;
  vtkRenderPass* RenderPass;

  bool Enabled;
  bool DisableIceT;
  int ImageReductionFactor;
  vtkOpenGLRenderer* Renderer;

  bool UseDepthBuffer;
  bool RenderEmptyImages;
  bool DataReplicatedOnAllProcesses;

private:
  vtkPVSynchronizedRenderer(const vtkPVSynchronizedRenderer&) = delete;
  void operator=(const vtkPVSynchronizedRenderer&) = delete;
};

#endif
