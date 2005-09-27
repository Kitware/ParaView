/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraKeyFrame.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCameraKeyFrame - Key frame for camera proxy.
// .SECTION Description
// This is the GUI for the Camera key frame.

#ifndef __vtkPVCameraKeyFrame_h
#define __vtkPVCameraKeyFrame_h

#include "vtkPVProxyKeyFrame.h"
class vtkKWApplication;
class vtkKWLabel;
class vtkKWPushButton;
class vtkKWThumbWheel;
class vtkSMDoubleVectorProperty;
class vtkSMProxy;

class VTK_EXPORT vtkPVCameraKeyFrame  : public vtkPVProxyKeyFrame
{
public:
  static vtkPVCameraKeyFrame* New();
  vtkTypeRevisionMacro(vtkPVCameraKeyFrame, vtkPVProxyKeyFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialized Key Value using current animated property value.
  virtual void InitializeKeyValueUsingCurrentState();

  // Description:
  // Initialize the Key Value bounds using current animatied property value
  // and domain state. Camera doesn't have much of a domain, 
  // hence we do nothing here.
  virtual void InitializeKeyValueDomainUsingCurrentState() { };

  // Description:
  // Camera doesn't have min/max values. Hence this implementation
  // does nothing.
  virtual void SetValueToMinimum() { }
  virtual void SetValueToMaximum() { }

  // Description:
  // Set the value of this keyframe using the state of the 
  // camera proxy. 
  virtual void SetKeyValue(vtkSMProxy* cameraProxy);
  void SetPosition(double x, double y, double z);
  void SetFocalPoint(double x, double y, double z);
  void SetViewUp(double x, double y, double z);
  void SetViewAngle(double angle);

  // Description:
  // Methods to set values but also adds it to the trace.
  void SetPositionWithTrace(double x, double y, double z);
  void SetFocalPointWithTrace(double x, double y, double z);
  void SetViewUpWithTrace(double x, double y, double z);
  void SetViewAngleWithTrace(double a);
  

  // Description:
  // Generic method to set a property.
  void SetProperty(const char* name, double val);
  void SetProperty(const char* name, double x, double y, double z);
  void SetProperty(const char* name ,vtkSMDoubleVectorProperty* sdvp);


  // Description:;
  // Callbacks for GUI.
  void CaptureCurrentCameraCallback();
  void PositionChangedCallback();
  void FocalPointChangedCallback();
  void ViewUpChangedCallback();
  void ViewAngleChangedCallback();

  // Description:
  // Save pvs state.
  virtual void SaveState(ofstream* file);

protected:
  vtkPVCameraKeyFrame();
  ~vtkPVCameraKeyFrame();

  // Description:
  // Generic method to set a property.
  void SetProperty(const char* name, const double* value);
  
  // Description:
  // Create the GUI for this type of keyframe.
  virtual void ChildCreate(vtkKWApplication* app);

  // Description:
  // Update the values from the vtkSMKeyFrameProxy.
  virtual void UpdateValuesFromProxy();

  vtkKWLabel* PositionLabel;
  vtkKWLabel* FocalPointLabel;
  vtkKWLabel* ViewUpLabel;
  vtkKWLabel* ViewAngleLabel;
  vtkKWPushButton* CaptureCurrentCamera;

  vtkKWThumbWheel* PositionWheels[3];
  vtkKWThumbWheel* FocalPointWheels[3];
  vtkKWThumbWheel* ViewUpWheels[3];
  vtkKWThumbWheel* ViewAngleWheel;

private:
  vtkPVCameraKeyFrame(const vtkPVCameraKeyFrame&); // Not implemented.
  void operator=(const vtkPVCameraKeyFrame&); // Not implemented.
};


#endif

