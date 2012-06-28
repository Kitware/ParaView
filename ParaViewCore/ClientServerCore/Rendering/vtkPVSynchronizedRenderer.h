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
// .NAME vtkPVSynchronizedRenderer - synchronizes and composites renderers among
// processes in ParaView configurations.
// .SECTION Description
//

#ifndef __vtkPVSynchronizedRenderer_h
#define __vtkPVSynchronizedRenderer_h

#include "vtkObject.h"

class vtkIceTSynchronizedRenderers;
class vtkImageProcessingPass;
class vtkPKdTree;
class vtkPVSession;
class vtkRenderer;
class vtkRenderPass;
class vtkSynchronizedRenderers;
class vtkOpenGLRenderer;

class VTK_EXPORT vtkPVSynchronizedRenderer : public vtkObject
{
public:
  static vtkPVSynchronizedRenderer* New();
  vtkTypeMacro(vtkPVSynchronizedRenderer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set this flag to true before calling Initialize() to disable using
  // vtkIceTSynchronizedRenderers for parallel rendering.
  vtkSetMacro(DisableIceT, bool);
  vtkGetMacro(DisableIceT, bool);

  // Must be called once to initialize the class. Id is uniquefier. It is
  // typically same as the id passed to vtkPVView::Initialize(). This makes it
  // possible to identify what view this instance corresponds to.
  // vtkPVSynchronizedRenderer passes this id to vtkIceTSynchronizedRenderers.
  // vtkIceTSynchronizedRenderers uses the id to ensure that the correct group
  // of views is shown on a tile-display.
  void Initialize(vtkPVSession* session, unsigned int id);

  // Description:
  // kd tree that gives processes ordering. Initial value is a NULL pointer.
  // This is used only when UseOrderedCompositing is true.
  void SetKdTree(vtkPKdTree *kdtree);

  // Description:
  // Set the renderer that is being synchronized.
  void SetRenderer(vtkRenderer*);

  // Description:
  // Enable/Disable parallel rendering.
  virtual void SetEnabled(bool enabled);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);

  // Description:
  // Get/Set the image reduction factor.
  // This needs to be set on all processes and must match up.
  void SetImageReductionFactor(int);
  vtkGetMacro(ImageReductionFactor, int);

  // Description:
  // Set to true if data is replicated on all processes. This will enable IceT
  // to minimize communications since data is available on all process. Off by
  // default.
  void SetDataReplicatedOnAllProcesses(bool);
  vtkBooleanMacro(DataReplicatedOnAllProcesses, bool);

  // Description:
  // Get/Set an image processing pass to process the rendered images.
  void SetImageProcessingPass(vtkImageProcessingPass*);
  vtkGetObjectMacro(ImageProcessingPass, vtkImageProcessingPass);

  // Description:
  // Get/Set geometry rendering pass. This pass is used to render the geometry.
  // If none specified then default rendering pipeline is used. This is
  // typically the render-pass pipeline after the CameraPass. The CameraPass is
  // setup by ParaView specially since ParaView needs some customizations for
  // multiviews/icet etc.
  void SetRenderPass(vtkRenderPass*);
  vtkGetObjectMacro(RenderPass, vtkRenderPass);

  // Description:
  // Passes the compressor configuration to the client-server synchronizer, if
  // any. This affects the image compression used to relay images back to the
  // client.
  // See vtkPVClientServerSynchronizedRenderers::ConfigureCompressor() for
  // details.
  void ConfigureCompressor(const char* configuration);
  void SetLossLessCompression(bool);

  // Description:
  // Activates or de-activated the use of Depth Buffer in an ImageProcessingPass
  void SetUseDepthBuffer(bool);

//BTX
protected:
  vtkPVSynchronizedRenderer();
  ~vtkPVSynchronizedRenderer();

  // Description:
  // Sets up the render passes on the renderer. This won't get called on
  // processes where vtkIceTSynchronizedRenderers is used. In that case the
  // passes are forwarded to the vtkIceTSynchronizedRenderers instance.
  virtual void SetupPasses();

  vtkSynchronizedRenderers* CSSynchronizer;
  vtkSynchronizedRenderers* ParallelSynchronizer;
  vtkImageProcessingPass *ImageProcessingPass;
  vtkRenderPass* RenderPass;

  enum ModeEnum
    {
    INVALID,
    BUILTIN,
    CLIENT,
    SERVER,
    BATCH
    };

  ModeEnum Mode;
  bool Enabled;
  bool DisableIceT;
  int ImageReductionFactor;
  vtkOpenGLRenderer* Renderer;

  bool UseDepthBuffer;
private:
  vtkPVSynchronizedRenderer(const vtkPVSynchronizedRenderer&); // Not implemented
  void operator=(const vtkPVSynchronizedRenderer&); // Not implemented
//ETX
};

#endif
