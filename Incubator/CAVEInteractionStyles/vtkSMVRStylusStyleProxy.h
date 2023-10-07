// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRStylusStyleProxy
 * @brief   an interaction style to control translation and rotation with a stylus
 *
 * vtkSMVRStylusStyleProxy is an interaction style that uses the position of the
 * stylus in screen space (for example the stylus of the zSpace) to modify a 4x4 matrix.
 * Only the location of the stylus is used, the rotation has no effect.
 * Only works with RenderView proxy.
 */
#ifndef vtkSMVRStylusStyleProxy_h
#define vtkSMVRStylusStyleProxy_h

#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRTrackStyleProxy.h"

struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRStylusStyleProxy
  : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRStylusStyleProxy* New();
  vtkTypeMacro(vtkSMVRStylusStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMVRStylusStyleProxy();
  ~vtkSMVRStylusStyleProxy() override = default;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool EnableTranslate;
  bool EnableRotate;

private:
  vtkSMVRStylusStyleProxy(const vtkSMVRStylusStyleProxy&) = delete;
  void operator=(const vtkSMVRStylusStyleProxy&) = delete;

  double LastRecordedPosition[3];
  bool PositionRecorded;
};

#endif // vtkSMVRStylusStyleProxy_h
