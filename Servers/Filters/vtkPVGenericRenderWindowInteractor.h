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

class VTK_EXPORT vtkPVGenericRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
  static vtkPVGenericRenderWindowInteractor *New();
  vtkTypeRevisionMacro(vtkPVGenericRenderWindowInteractor, vtkRenderWindowInteractor);
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
  // Since I was having trouble with the 3D widget doing the final full res render,
  // setting this to 0 causes the render view to eventually render.
  void SetInteractiveRenderEnabled(int);
  vtkGetMacro(InteractiveRenderEnabled,int);
  vtkBooleanMacro(InteractiveRenderEnabled,int);
  virtual void Render();

  // Description:
  // Methods broadcasted to the satellites to synchronize 3D widgets.
  void SatelliteLeftPress(int x, int y, int control, int shift);
  void SatelliteMiddlePress(int x, int y, int control, int shift);
  void SatelliteRightPress(int x, int y, int control, int shift);
  void SatelliteLeftRelease(int x, int y, int control, int shift);
  void SatelliteMiddleRelease(int x, int y, int control, int shift);
  void SatelliteRightRelease(int x, int y, int control, int shift);
  void SatelliteMove(int x, int y);
  void SatelliteKeyPress(char keyCode, int x, int y);

protected:
  vtkPVGenericRenderWindowInteractor();
  ~vtkPVGenericRenderWindowInteractor();

  int CalculateReductionFactor(int winSize1, int renSize1);
  
  vtkPVRenderViewProxy *PVRenderView;
  int ReductionFactor;
  int InteractiveRenderEnabled;
  vtkRenderer* Renderer;

private:
  vtkPVGenericRenderWindowInteractor(const vtkPVGenericRenderWindowInteractor&); // Not implemented
  void operator=(const vtkPVGenericRenderWindowInteractor&); // Not implemented
};

#endif
