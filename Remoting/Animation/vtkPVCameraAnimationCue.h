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
/**
 * @class   vtkPVCameraAnimationCue
 *
 * vtkPVCameraAnimationCue is a specialization of the vtkPVKeyFrameAnimationCue suitable
 * for animating cameras from a vtkPVRenderView.
*/

#ifndef vtkPVCameraAnimationCue_h
#define vtkPVCameraAnimationCue_h

#include "vtkPVKeyFrameAnimationCue.h"
#include "vtkRemotingAnimationModule.h" //needed for exports

class vtkCamera;
class vtkPVCameraCueManipulator;
class vtkPVRenderView;
class vtkSMProxy;

class VTKREMOTINGANIMATION_EXPORT vtkPVCameraAnimationCue : public vtkPVKeyFrameAnimationCue
{
public:
  static vtkPVCameraAnimationCue* New();
  vtkTypeMacro(vtkPVCameraAnimationCue, vtkPVKeyFrameAnimationCue);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the render view.
   */
  void SetView(vtkPVRenderView*);
  vtkGetObjectMacro(View, vtkPVRenderView);
  //@}

  /**
   * Returns the animated camera, if any.
   */
  vtkCamera* GetCamera();

  //@{
  /**
   * Forwarded to vtkPVCameraCueManipulator.
   */
  void SetMode(int mode);
  void SetInterpolationMode(int mode);
  //@}

  //@{
  /**
   * Get/Set the animation timekeeper
   */
  vtkGetObjectMacro(TimeKeeper, vtkSMProxy);
  void SetTimeKeeper(vtkSMProxy*);
  //@}

  void BeginUpdateAnimationValues() override {}
  void SetAnimationValue(int, double) override {}
  void EndUpdateAnimationValues() override;

  void SetDataSourceProxy(vtkSMProxy* dataSourceProxy);

protected:
  vtkPVCameraAnimationCue();
  ~vtkPVCameraAnimationCue() override;

  vtkPVRenderView* View;
  vtkSMProxy* DataSourceProxy;
  vtkSMProxy* TimeKeeper;

private:
  vtkPVCameraAnimationCue(const vtkPVCameraAnimationCue&) = delete;
  void operator=(const vtkPVCameraAnimationCue&) = delete;
};

#endif
