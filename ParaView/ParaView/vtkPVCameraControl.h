/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraControl.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCameraControl
// .SECTION Description

#ifndef __vtkPVCameraControl_h
#define __vtkPVCameraControl_h

#include "vtkKWWidget.h"

class vtkKWEntry;
class vtkKWLabel;
class vtkKWPushButton;
class vtkPVInteractorStyleCenterOfRotation;
class vtkPVRenderView;

class VTK_EXPORT vtkPVCameraControl : public vtkKWWidget
{
public:
  static vtkPVCameraControl* New();
  vtkTypeRevisionMacro(vtkPVCameraControl, vtkKWWidget);
  void PrintSelf(ostream &os, vtkIndent indent);
  
  void SetInteractorStyle(vtkPVInteractorStyleCenterOfRotation *style);
  void SetRenderView(vtkPVRenderView *view);
  
  void Create(vtkKWApplication *app, const char *args);
  
  void ElevationButtonCallback();
  void AzimuthButtonCallback();
  void RollButtonCallback();
  
protected:
  vtkPVCameraControl();
  ~vtkPVCameraControl();
  
  vtkPVInteractorStyleCenterOfRotation *InteractorStyle;
  vtkPVRenderView *RenderView;
  
  void Elevation(double angle);
  void Azimuth(double angle);
  void Roll(double angle);
  
  vtkKWPushButton *ElevationButton;
  vtkKWEntry *ElevationEntry;
  vtkKWLabel *ElevationLabel;
  
  vtkKWPushButton *AzimuthButton;
  vtkKWEntry *AzimuthEntry;
  vtkKWLabel *AzimuthLabel;

  vtkKWPushButton *RollButton;
  vtkKWEntry *RollEntry;
  vtkKWLabel *RollLabel;
  
private:
  vtkPVCameraControl(const vtkPVCameraControl&);  // Not implemented
  void operator=(const vtkPVCameraControl&);  // Not implemented
};

#endif
