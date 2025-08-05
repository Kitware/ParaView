// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRControlLocationRelativeStyleProxy
 * @brief   an interaction style to control the position of a point with a stylus
 *
 * vtkSMVRControlLocationRelativeStyleProxy is an interaction style that uses the position of the
 * tracker in screen space to modify the position of a 3D point.
 */
#ifndef vtkSMVRControlLocationRelativeStyleProxy_h
#define vtkSMVRControlLocationRelativeStyleProxy_h

#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRTrackStyleProxy.h"

struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRControlLocationRelativeStyleProxy
  : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRControlLocationRelativeStyleProxy* New();
  vtkTypeMacro(vtkSMVRControlLocationRelativeStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetControlledPropertySize() override { return 3; }

  // Overridden to defer expensive calculations and update vtk objects
  bool Update() override;

protected:
  vtkSMVRControlLocationRelativeStyleProxy();
  ~vtkSMVRControlLocationRelativeStyleProxy() override = default;

  void HandleButton(const vtkVREvent& data) override;
  void HandleTracker(const vtkVREvent& data) override;

private:
  vtkSMVRControlLocationRelativeStyleProxy(
    const vtkSMVRControlLocationRelativeStyleProxy&) = delete;
  void operator=(const vtkSMVRControlLocationRelativeStyleProxy&) = delete;

  bool EnableMovePoint;
  double OriginalPoint[3];
  bool FirstUpdate;
  double LastRecordedPosition[3];
  bool PositionRecorded;

  vtkNew<vtkMatrix4x4> TrackerMatrix;
};

#endif // vtkSMVRControlLocationRelativeStyleProxy_h
