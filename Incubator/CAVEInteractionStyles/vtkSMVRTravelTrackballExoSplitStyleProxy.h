// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRTravelTrackballExoSplitStyleProxy
 * @brief   an interaction style to control translation and rotation with a stylus
 *
 * vtkSMVRTravelTrackballExoSplitStyleProxy is an interaction style that uses the position of the
 * stylus in screen space (for example the stylus of the zSpace) to modify a 4x4 matrix.
 * Only the location of the stylus is used, the rotation has no effect.
 * Only works with RenderView proxy.
 */
#ifndef vtkSMVRTravelTrackballExoSplitStyleProxy_h
#define vtkSMVRTravelTrackballExoSplitStyleProxy_h

#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRTrackStyleProxy.h"
#include "vtkTransform.h"

struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRTravelTrackballExoSplitStyleProxy
  : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRTravelTrackballExoSplitStyleProxy* New();
  vtkTypeMacro(vtkSMVRTravelTrackballExoSplitStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Overridden to defer expensive calculations and update vtk objects
  bool Update() override;

protected:
  vtkSMVRTravelTrackballExoSplitStyleProxy();
  ~vtkSMVRTravelTrackballExoSplitStyleProxy() override = default;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

private:
  vtkSMVRTravelTrackballExoSplitStyleProxy(
    const vtkSMVRTravelTrackballExoSplitStyleProxy&) = delete;
  void operator=(const vtkSMVRTravelTrackballExoSplitStyleProxy&) = delete;

  double LastRecordedPosition[3];
  bool PositionRecorded;
  bool EnableTranslate;
  bool EnableRotate;

  vtkNew<vtkMatrix4x4> TrackerMatrix;
  vtkNew<vtkMatrix4x4> TransformMatrix;
  vtkNew<vtkTransform> Transform;
  vtkNew<vtkMatrix4x4> ModelMatrix;
  vtkNew<vtkMatrix4x4> ModelViewMatrix;
  vtkNew<vtkMatrix4x4> InvViewMatrix;
};

#endif // vtkSMVRTravelTrackballExoSplitStyleProxy_h
