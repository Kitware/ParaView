// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRMovePointStyleProxy
 * @brief   an interaction style to control the position of a point with a stylus
 *
 * vtkSMVRMovePointStyleProxy is an interaction style that uses the position of the
 * tracker in screen space to modify the position of a 3D point.
 */
#ifndef vtkSMVRMovePointStyleProxy_h
#define vtkSMVRMovePointStyleProxy_h

#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRTrackStyleProxy.h"

struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRMovePointStyleProxy
  : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRMovePointStyleProxy* New();
  vtkTypeMacro(vtkSMVRMovePointStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetControlledPropertySize() override { return 3; }

protected:
  vtkSMVRMovePointStyleProxy();
  ~vtkSMVRMovePointStyleProxy() override = default;

  void HandleButton(const vtkVREvent& data) override;
  void HandleTracker(const vtkVREvent& data) override;

  bool EnableMovePoint;

private:
  vtkSMVRMovePointStyleProxy(const vtkSMVRMovePointStyleProxy&) = delete;
  void operator=(const vtkSMVRMovePointStyleProxy&) = delete;

  double LastRecordedPosition[3];
  bool PositionRecorded;
};

#endif // vtkSMVRMovePointStyleProxy_h
