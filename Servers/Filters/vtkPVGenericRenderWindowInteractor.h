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

class VTK_EXPORT vtkPVGenericRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  static vtkPVGenericRenderWindowInteractor *New();
  vtkTypeMacro(vtkPVGenericRenderWindowInteractor, vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetPVRenderView(vtkPVRenderViewProxy *view);
  vtkGetObjectMacro(PVRenderView, vtkPVRenderViewProxy);

  // Description:
  // Fire various events, SetEventInformation should be called just prior
  // to calling any of these methods.  This methods will Invoke the 
  // corresponding vtk event.
  virtual void ConfigureEvent();

  // Description:
  // My sollution to the poked renderer problem.
  // This interactor class always returns this renderer as poked render.
  // This insures the 2D renderer will never be poked.
  void SetRenderer(vtkRenderer *view);
  vtkGetObjectMacro(Renderer,vtkRenderer);
  virtual vtkRenderer *FindPokedRenderer(int,int);

  // Description:
  // Set the event onformation, but remember keys from before.
  void SetMoveEventInformationFlipY(int x, int y);

  // Description:
  // 3D widgets call render on this interactor directly.
  // They call SetInteractive to tell whether to use still or interactive rendering.
  // This class just forwards the render request to ParaView's RenderModule.
  // DesiredUpdateRate is ignored.
  void SetInteractiveRenderEnabled(int);
  vtkGetMacro(InteractiveRenderEnabled,int);
  vtkBooleanMacro(InteractiveRenderEnabled,int);

  // Description:
  // Triggers a render.
  virtual void Render();

  // Description:
  // Methods broadcasted to the satellites to synchronize 3D widgets.
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

  double CenterOfRotation[3];

private:
  vtkPVGenericRenderWindowInteractor(const vtkPVGenericRenderWindowInteractor&); // Not implemented
  void operator=(const vtkPVGenericRenderWindowInteractor&); // Not implemented

  vtkPVGenericRenderWindowInteractorObserver* Observer;
};

#endif
