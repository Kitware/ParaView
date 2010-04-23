/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTCompositeViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIceTCompositeViewProxy
// .SECTION Description
// This is a render view that uses IceT for compositing. It does not support
// tiles or client-server scenarios. Look at the subclasses for such cases. As
// far as this view module is concerned CLIENT == RENDER_SERVER_ROOT.

#ifndef __vtkSMIceTCompositeViewProxy_h
#define __vtkSMIceTCompositeViewProxy_h

#include "vtkSMMultiProcessRenderView.h"

class vtkInformationObjectBaseKey;
class vtkSMRepresentationStrategyVector;

class VTK_EXPORT vtkSMIceTCompositeViewProxy : public vtkSMMultiProcessRenderView
{
public:
  static vtkSMIceTCompositeViewProxy* New();
  vtkTypeMacro(vtkSMIceTCompositeViewProxy, vtkSMMultiProcessRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Keys used to communicate information to representations/strategies.
  static vtkInformationObjectBaseKey* KD_TREE();

  // Description:
  // Overridden to send the GUISize to the MultiViewManager.
  virtual void SetGUISize(int x, int y);

  // Description:
  // Overridden to send the ViewPosition to the MultiViewManager.
  virtual void SetViewPosition(int x, int y);

  // Description:
  // Set the view size.
  // Default implementation passes the size to the MultiViewManager.
  virtual void SetViewSize(int width, int height);

  // Description:
  // Get/Set the image reduction factor used for compositing parallel images in
  // InteractiveRender. ImageReductionFactor=1 implies no reduction at all.
  vtkSetClampMacro(ImageReductionFactor, int, 1, 100);
  vtkGetMacro(ImageReductionFactor, int);

  // Description:
  // When set, ordered compositing is disabled even when requested by
  // representation strategies. Typically ordered compositing is needed when
  // rendering representations that have opactity < 1.0 or volume rendering.
  // By default, set to false.
  vtkSetMacro(DisableOrderedCompositing, int);
  vtkGetMacro(DisableOrderedCompositing, int);

  // Description:
  // Returns an image data that contains a "screenshot" of the window.
  // It is the responsibility of the caller to delete the image data.
  virtual vtkImageData* CaptureWindow(int magnification);

//BTX
protected:
  vtkSMIceTCompositeViewProxy();
  ~vtkSMIceTCompositeViewProxy();

  // Description:
  // Pre-CreateVTKObjects initialization.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // Post-CreateVTKObjects initialization.
  virtual void EndCreateVTKObjects();

  // Description:
  // Called by RemoveRepresentation(). Subclasses can override to remove 
  // observers etc.
  virtual void RemoveRepresentationInternal(vtkSMRepresentationProxy* rep);

  // Description:
  // Method called before Still Render is called.
  // Used to perform some every-still-render-setup actions.
  virtual void BeginStillRender();

  // Description:
  // Method called before Interactive Render.
  // Used to perform some every-interactive-render-setup actions.
  virtual void BeginInteractiveRender();

  // Description:
  // In multiview setups, some viewmodules may share certain objects with each
  // other. This method is used in such cases to give such views an opportunity
  // to share those objects.
  // Default implementation is empty.
  virtual void InitializeForMultiView(vtkSMViewProxy* otherView);

  // Description:
  // Pass compositing decision to the parallel render manager and the view
  // helper.
  virtual void SetUseCompositing(bool usecompositing);

  // Description:
  // Pass the image actual reduction factor to use on to the
  // ParallelRenderManager.
  virtual void SetImageReductionFactorInternal(int factor);

  // Description:
  // This method should update the KdTree used for ordered compositing. This
  // method should also verify if ordered compositing is needed at all and
  // update the renderer accordingly.
  void UpdateOrderedCompositingPipeline();

  // Description:
  // Pass ordered compositing decision to all strategies.
  void SetOrderedCompositingDecision(bool decision);


  void UpdateViewport();

  // Manager used for multiview.
  vtkSMProxy* MultiViewManager;
  vtkClientServerID SharedMultiViewManagerID;

  // Parallel render manager that manager rendering in parallel.
  vtkSMProxy* ParallelRenderManager;
  vtkClientServerID SharedParallelRenderManagerID;

  // Used when ordered compositing is needed.
  vtkSMProxy* KdTree;

  // Used to generate the KdTree.
  vtkSMProxy* KdTreeManager;

  // Reduction factor used for compositing when using interactive render.
  int ImageReductionFactor;

  int DisableOrderedCompositing;
  bool LastOrderedCompositingDecision;

  // These need to initialized for IceT rendering. However, this class does not
  // provide public API to change these, since this view does not support
  // tiling.
  int TileDimensions[2];
  int TileMullions[2];
  bool EnableTiles;

  int ViewSize[2];
  bool ViewSizeInitialized;

  vtkSMRepresentationStrategyVector *ActiveStrategyVector;

  // ID used to identify renders with the MultiViewManager.
  int RenderersID;

  // A flag that can be turned off by subclasses when they do not 
  // drive vtkIceTRenderers. In that case we do not want to call nonexistant
  // methods related to vtkIceTRenderer
  bool UsingIceTRenderers;
private:
  vtkSMIceTCompositeViewProxy(const vtkSMIceTCompositeViewProxy&); // Not implemented
  void operator=(const vtkSMIceTCompositeViewProxy&); // Not implemented
//ETX
};

#endif

