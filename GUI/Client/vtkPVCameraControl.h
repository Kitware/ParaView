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
// .NAME vtkPVCameraControl - a control widget for manipulating camera
//
// .SECTION Description
// This widget defines a user interface for controlling the camera.

#ifndef __vtkPVCameraControl_h
#define __vtkPVCameraControl_h

#include "vtkPVTracedWidget.h"

class vtkKWEntry;
class vtkKWLabel;
class vtkKWPushButton;
class vtkPVInteractorStyleCenterOfRotation;
class vtkPVRenderView;

class VTK_EXPORT vtkPVCameraControl : public vtkPVTracedWidget
{
public:
  static vtkPVCameraControl* New();
  vtkTypeRevisionMacro(vtkPVCameraControl, vtkPVTracedWidget);
  void PrintSelf(ostream &os, vtkIndent indent);
  
  // Description:
  // Set the center of rotation interactor style.  This is used for getting
  // the current center of rotation.
  void SetInteractorStyle(vtkPVInteractorStyleCenterOfRotation *style);
  
  // Description:
  // Set the render view to operate in.
  void SetRenderView(vtkPVRenderView *view);
  
  // Desription:
  // Create the widget.
  void Create(vtkKWApplication *app, const char *args);
  
  // Description:
  // Callbacks for the 3 buttons.
  void ElevationButtonCallback();
  void AzimuthButtonCallback();
  void RollButtonCallback();
  
  // Description:
  // Made public so they can be called from a script
  void Elevation(double angle);
  void Azimuth(double angle);
  void Roll(double angle);
  
protected:
  vtkPVCameraControl();
  ~vtkPVCameraControl();
  
  vtkPVInteractorStyleCenterOfRotation *InteractorStyle;
  vtkPVRenderView *RenderView;
  
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
