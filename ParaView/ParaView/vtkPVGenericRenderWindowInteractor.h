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

#include "vtkGenericRenderWindowInteractor.h"

class vtkPVRenderView;

class VTK_EXPORT vtkPVGenericRenderWindowInteractor : public vtkGenericRenderWindowInteractor
{
public:
  static vtkPVGenericRenderWindowInteractor *New();
  vtkTypeRevisionMacro(vtkPVGenericRenderWindowInteractor, vtkGenericRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetPVRenderView(vtkPVRenderView *view);
  vtkGetObjectMacro(PVRenderView, vtkPVRenderView);

  // Description:
  // Set the event onformation, but remember keys from before.
  void SetMoveEventInformationFlipY(int x, int y);


  // Description:
  // 3D widgets call render on this interactor directly.
  // They call SetInteractive to tell whether to use still or interactive rendering.
  // This class just forwards the render request to ParaView's RenderModule.
  // DesiredUpdateRate is ignored.
  vtkSetMacro(InteractiveRenderEnabled,int);
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

protected:
  vtkPVGenericRenderWindowInteractor();
  ~vtkPVGenericRenderWindowInteractor();

  int CalculateReductionFactor(int winSize1, int renSize1);
  
  vtkPVRenderView *PVRenderView;
  int ReductionFactor;
  int InteractiveRenderEnabled;

private:
  vtkPVGenericRenderWindowInteractor(const vtkPVGenericRenderWindowInteractor&); // Not implemented
  void operator=(const vtkPVGenericRenderWindowInteractor&); // Not implemented
};

#endif
