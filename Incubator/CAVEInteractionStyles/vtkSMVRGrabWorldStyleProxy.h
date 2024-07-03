// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRGrabWorldStyleProxy_h
#define vtkSMVRGrabWorldStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRTrackStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRGrabWorldStyleProxy
  : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRGrabWorldStyleProxy* New();
  vtkTypeMacro(vtkSMVRGrabWorldStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMVRGrabWorldStyleProxy();
  ~vtkSMVRGrabWorldStyleProxy() override;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool EnableTranslate; /* mirrors the button assigned the "Translate World" role */
  bool EnableRotate;    /* mirrors the button assigned the "Rotate World" role */

  bool IsInitialTransRecorded; /* flag indicating that we're in the middle of a translation
                                  operation */
  bool IsInitialRotRecorded; /* flag indicating that we're in the middle of a rotation operation */
                             /* NOTE: only one of translation or rotation can be active at a time */

  vtkNew<vtkMatrix4x4> InverseInitialTransMatrix;
  vtkNew<vtkMatrix4x4> InverseInitialRotMatrix;

  vtkNew<vtkMatrix4x4> CachedTransMatrix;
  vtkNew<vtkMatrix4x4> CachedRotMatrix;

private:
  vtkSMVRGrabWorldStyleProxy(const vtkSMVRGrabWorldStyleProxy&) = delete; // Not implemented
  void operator=(const vtkSMVRGrabWorldStyleProxy&) = delete;             // Not implemented

  float GetSpeedFactor(vtkCamera* cam, vtkMatrix4x4* mvmatrix); /* WRS: what does this do? */
};

#endif // vtkSMVRGrabWorldStyleProxy_h
