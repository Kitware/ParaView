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
// .NAME vtkPVRenderView
// .SECTION Description
// vtkRenderViewBase equivalent that is specialized for ParaView. Eventually
// vtkRenderViewBase should have a abstract base-class that this will derive from
// instead of vtkRenderViewBase since we do not use the labelling/icon stuff from
// vtkRenderViewBase.

// FIXME: Following is temporary -- until I decide if that's necessary at all.
// vtkPVRenderView has two types of public methods:
// 1. @CallOnAllProcessess -- must be called on all processes with exactly the
//                            same values.
// 2. @CallOnClientOnly    -- can be called only on the "client" process. These
//                            typically encapsulate client-side logic such as
//                            deciding if we are doing remote rendering or local
//                            rendering etc.

// Utkarsh: Try to use methods that will be called on all processes for most
// decision making similar to what ResetCamera() does. This will avoid the need
// to have special code in vtkSMRenderViewProxy and will simplify life when
// creating new views. "Move logic to VTK" -- that's the Mantra.
#ifndef __vtkPVRenderView_h
#define __vtkPVRenderView_h

#include "vtkPVView.h"

class vtkCamera;
class vtkPVSynchronizedRenderer;
class vtkPVSynchronizedRenderWindows;
class vtkRenderViewBase;
class vtkRenderer;
class vtkRenderWindow;

class VTK_EXPORT vtkPVRenderView : public vtkView
{
public:
  static vtkPVRenderView* New();
  vtkTypeMacro(vtkPVRenderView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // @CallOnAllProcessess
  void Initialize(unsigned int id);

  // Description:
  // Set the position on this view in the multiview configuration.
  // This can be called only after Initialize().
  // @CallOnAllProcessess
  void SetPosition(int, int);

  // Description:
  // Set the size of this view in the multiview configuration.
  // This can be called only after Initialize().
  // @CallOnAllProcessess
  void SetSize(int, int);

  // Description:
  // Gets the non-composited renderer for this view. This is typically used for
  // labels, 2D annotations etc.
  // @CallOnAllProcessess
  vtkGetObjectMacro(NonCompositedRenderer, vtkRenderer);

  // Description:
  // Get the active camera.
  vtkCamera* GetActiveCamera();

  // Description:
  // Returns the render window.
  vtkRenderWindow* GetRenderWindow();

  // Description:
  // Resets the active camera using collective prop-bounds.
  // @CallOnAllProcessess
  void ResetCamera();

  // Description:
  // Triggers a high-resolution render.
  // @CallOnAllProcessess
  virtual void StillRender();

  // Description:
  // Triggers a interactive render. Based on the settings on the view, this may
  // result in a low-resolution rendering or a simplified geometry rendering.
  // @CallOnAllProcessess
  virtual void InteractiveRender();

  // Description:
  // Get/Set the reduction-factor to use when for StillRender(). This is
  // typically set to 1, but in some cases with terrible connectivity or really
  // large displays, one may want to use a sub-sampled image even for
  // StillRender(). This is set it number of pixels to be sub-sampled by.
  // Note that image reduction factors have no effect when in built-in mode.
  // @CallOnAllProcessess
  vtkSetClampMacro(StillRenderImageReductionFactor, int, 1, 20);
  vtkGetMacro(StillRenderImageReductionFactor, int);

  // Description:
  // Get/Set the reduction-factor to use when for InteractiveRender().
  // This is set it number of pixels to be sub-sampled by.
  // Note that image reduction factors have no effect when in built-in mode.
  // @CallOnAllProcessess
  vtkSetClampMacro(InteractiveRenderImageReductionFactor, int, 1, 20);
  vtkGetMacro(InteractiveRenderImageReductionFactor, int);

  // Description:
  // Get/Set the data-size in kilobytes above which remote-rendering should be
  // used, if possible.
  // @CallOnAllProcessess
  vtkSetMacro(RemoteRenderingThreshold, unsigned long);
  vtkGetMacro(RemoteRenderingThreshold, unsigned long);

//BTX
protected:
  vtkPVRenderView();
  ~vtkPVRenderView();

  // Description:
  // Sychronizes the geometry size information on all nodes.
  // @CallOnAllProcessess
  void GatherGeometrySizeInformation();

  // Description:
  // Returns true if distributed rendering should be used.
  bool GetUseDistributedRendering();

  vtkRenderViewBase* RenderView;
  vtkRenderer* NonCompositedRenderer;
  vtkPVSynchronizedRenderWindows* SynchronizedWindows;
  vtkPVSynchronizedRenderer* SynchronizedRenderers;
  unsigned int Identifier;

  int StillRenderImageReductionFactor;
  int InteractiveRenderImageReductionFactor;

  unsigned long GeometrySize;
  unsigned long RemoteRenderingThreshold;
private:
  vtkPVRenderView(const vtkPVRenderView&); // Not implemented
  void operator=(const vtkPVRenderView&); // Not implemented
//ETX
};

#endif
