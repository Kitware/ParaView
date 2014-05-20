/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraAnimationCue.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCameraAnimationCue
// .SECTION Description
// vtkPVCameraAnimationCue is a specialization of the vtkPVKeyFrameAnimationCue suitable
// for animating cameras from a vtkPVRenderView.

#ifndef __vtkPVCameraAnimationCue_h
#define __vtkPVCameraAnimationCue_h

#include "vtkPVAnimationModule.h" //needed for exports
#include "vtkPVKeyFrameAnimationCue.h"

class vtkCamera;
class vtkPVCameraCueManipulator;
class vtkPVRenderView;
class vtkSMProxy;

class VTKPVANIMATION_EXPORT vtkPVCameraAnimationCue : public vtkPVKeyFrameAnimationCue
{
public:
  static vtkPVCameraAnimationCue* New();
  vtkTypeMacro(vtkPVCameraAnimationCue, vtkPVKeyFrameAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the render view.
  void SetView(vtkPVRenderView*);
  vtkGetObjectMacro(View, vtkPVRenderView);

  // Description:
  // Returns the animated camera, if any.
  vtkCamera* GetCamera();

  // Description:
  // Forwarded to vtkPVCameraCueManipulator.
  void SetMode(int mode);

  virtual void BeginUpdateAnimationValues() {}
  virtual void SetAnimationValue(int, double){}
  virtual void EndUpdateAnimationValues();

  void SetDataSourceProxy(vtkSMProxy *dataSourceProxy);
//BTX
protected:
  vtkPVCameraAnimationCue();
  ~vtkPVCameraAnimationCue();

  vtkPVRenderView* View;
  vtkSMProxy* DataSourceProxy;
private:
  vtkPVCameraAnimationCue(const vtkPVCameraAnimationCue&); // Not implemented
  void operator=(const vtkPVCameraAnimationCue&); // Not implemented
//ETX
};

#endif
