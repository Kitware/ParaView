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

#include "vtkSMRenderViewProxy.h"

class vtkSMRepresentationStrategyVector;

class VTK_EXPORT vtkSMIceTCompositeViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMIceTCompositeViewProxy* New();
  vtkTypeRevisionMacro(vtkSMIceTCompositeViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // Get/Set the threshold used to determine if the compositing should be used
  // for rendering. In client-server views, this typically implies remote
  // render with compositing.
  vtkSetClampMacro(CompositeThreshold, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(CompositeThreshold, double);

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
  // In multiview configurations, all the render views must share the same
  // instance of the parallel render manager. Use this method to set that shared
  // instance. It must be set before calling CreateVTKObjects() on the view
  // proxy.
  void SetSharedParallelRenderManager(vtkSMProxy*);

  // Description:
  // In mutlview configurations, all the render views must share the same
  // instance of the multiview manager. Use this method to set that shared
  // instance. It must be set before calling CreateVTKObjects() on the view
  // proxy.
  void SetSharedMultiViewManager(vtkSMProxy*);

  // Description:
  // In mutlview configurations, all the render views must share the same
  // instance of the render window on the server side. 
  // Use this method to set that shared
  // instance. It must be set before calling CreateVTKObjects() on the view
  // proxy.
  void SetSharedRenderWindow(vtkSMProxy*);

//BTX
protected:
  vtkSMIceTCompositeViewProxy();
  ~vtkSMIceTCompositeViewProxy();

  // Description:
  // Pre-CreateVTKObjects initialization.
  virtual bool BeginCreateVTKObjects(int numObjects);

  // Description:
  // Post-CreateVTKObjects initialization.
  virtual void EndCreateVTKObjects(int numObjects);

  // Description:
  // Creates a new vtkSMRepresentationStrategy subclass based on the type
  // requested.
  virtual vtkSMRepresentationStrategy* NewStrategyInternal(
    int dataType);

  // Description:
  // Method called before Still Render is called.
  // Used to perform some every-still-render-setup actions.
  virtual void BeginStillRender();

  // Description:
  // Method called before Interactive Render.
  // Used to perform some every-interactive-render-setup actions.
  virtual void BeginInteractiveRender();

  // Indicates if we should render using compositing.
  // Returns true if compositing should be used, otherwise false.
  // Flag stillRender is set when this decision is to be made during StillRender
  // else it's 0 (for InteractiveRender);
  virtual bool GetCompositingDecision(
    unsigned long totalMemory, int stillRender);

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

  // Manager used for multiview.
  vtkSMProxy* MultiViewManager;
  vtkSMProxy* SharedMultiViewManager;

  // Parallel render manager that manager rendering in parallel.
  vtkSMProxy* ParallelRenderManager;
  vtkSMProxy* SharedParallelRenderManager;

  // Used when ordered compositing is needed.
  vtkSMProxy* KdTree;

  // Used to generate the KdTree.
  vtkSMProxy* KdTreeManager;

  // Render window shared on the server side.
  vtkSMProxy* SharedRenderWindow;

  // Reduction factor used for compositing when using interactive render.
  int ImageReductionFactor;

  int DisableOrderedCompositing;

  double CompositeThreshold;

  bool LastCompositingDecision;

  // These need to initialized for IceT rendering. However, this class does not
  // provide public API to change these, since this view does not support
  // tiling.
  int TileDimensions[2];
  int TileMullions[2];

  vtkSMRepresentationStrategyVector *ActiveStrategyVector;

private:
  vtkSMIceTCompositeViewProxy(const vtkSMIceTCompositeViewProxy&); // Not implemented
  void operator=(const vtkSMIceTCompositeViewProxy&); // Not implemented
//ETX
};

#endif

