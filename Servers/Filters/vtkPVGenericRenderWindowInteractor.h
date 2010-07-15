/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGenericRenderWindowInteractor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGenericRenderWindowInteractor
// .SECTION Description

#ifndef __vtkPVGenericRenderWindowInteractor_h
#define __vtkPVGenericRenderWindowInteractor_h

#include "vtkRenderWindowInteractor.h"

class vtkPVRenderViewProxy;
class vtkRenderer;
class vtkPVGenericRenderWindowInteractorObserver;
class vtkPVGenericRenderWindowInteractorTimer;

class VTK_EXPORT vtkPVGenericRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  static vtkPVGenericRenderWindowInteractor *New();
  vtkTypeMacro(vtkPVGenericRenderWindowInteractor, vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetPVRenderView(vtkPVRenderViewProxy *view);
  vtkGetObjectMacro(PVRenderView, vtkPVRenderViewProxy);

  // Description:
  // My sollution to the poked renderer problem.
  // This interactor class always returns this renderer as poked render.
  // This insures the 2D renderer will never be poked.
  void SetRenderer(vtkRenderer *view);
  vtkGetObjectMacro(Renderer,vtkRenderer);
  virtual vtkRenderer *FindPokedRenderer(int,int);

  // Description:
  // 3D widgets call render on this interactor directly.
  // They call SetInteractive to tell whether to use still or interactive rendering.
  // This class just forwards the render request to ParaView's RenderModule.
  // DesiredUpdateRate is ignored.
  void SetInteractiveRenderEnabled(int);
  vtkGetMacro(InteractiveRenderEnabled,int);
  vtkBooleanMacro(InteractiveRenderEnabled,int);

  // Description:
  // vtkPVGenericRenderWindowInteractor allows applications to support
  // "delayed-switch-to-non-interative-render" mode i.e. when user stops
  // interacting, the application does not want the scene to be immediately
  // rendered in non-interactive mode, but wait for a few seconds. This will
  // allow the user to do multiple adjustments while staying locked in the
  // interactive mode. For that, the application must first set
  // SetNonInteractiveRenderDelay(unsigned long milliseconds). If
  // milliseconds==0, then the application switches to non-interactive mode
  // immediately.
  // Note, currently delayed render is only supported when compiled with
  // VTK_USE_QVTK set to ON.
  vtkSetMacro(NonInteractiveRenderDelay, unsigned long);
  vtkGetMacro(NonInteractiveRenderDelay, unsigned long);

  // Description:
  // Triggers a render.
  virtual void Render();

  // Description:
  // These methods merely call SetEventInformation() and then fire the
  // appropriate vtk-event.
  virtual void OnLeftPress(int x, int y, int control, int shift);
  virtual void OnMiddlePress(int x, int y, int control, int shift);
  virtual void OnRightPress(int x, int y, int control, int shift);
  virtual void OnLeftRelease(int x, int y, int control, int shift);
  virtual void OnMiddleRelease(int x, int y, int control, int shift);
  virtual void OnRightRelease(int x, int y, int control, int shift);
  virtual void OnMove(int x, int y);
  virtual void OnKeyPress(char keyCode, int x, int y);


  // Overridden to pass interaction events from the interactor style out.
  virtual void SetInteractorStyle(vtkInteractorObserver *);

  // Description:
  // Propagates the center to the interactor style.
  // Currently, center of rotation is propagated only with the 
  // interactor style is a vtkPVInteractorStyle or subclass.
  vtkGetVector3Macro(CenterOfRotation, double);
  void SetCenterOfRotation(double x, double y, double z);
  void SetCenterOfRotation(double xyz[3])
    {
    this->SetCenterOfRotation(xyz[0], xyz[1], xyz[2]);
    }
protected:
  vtkPVGenericRenderWindowInteractor();
  ~vtkPVGenericRenderWindowInteractor();

  vtkPVRenderViewProxy *PVRenderView;
  int InteractiveRenderEnabled;
  vtkRenderer* Renderer;

  unsigned long NonInteractiveRenderDelay;
  double CenterOfRotation[3];

private:
  vtkPVGenericRenderWindowInteractor(const vtkPVGenericRenderWindowInteractor&); // Not implemented
  void operator=(const vtkPVGenericRenderWindowInteractor&); // Not implemented

  friend class vtkPVGenericRenderWindowInteractorTimer;
  friend class vtkPVGenericRenderWindowInteractorObserver;

  vtkPVGenericRenderWindowInteractorTimer* Timer;
  vtkPVGenericRenderWindowInteractorObserver* Observer;

  bool ForceInteractiveRender;
  vtkSetMacro(ForceInteractiveRender, bool);
  void DisableInteractiveRenderInternal();
  bool InteractiveRenderHappened;
};

#endif
