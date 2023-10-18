// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRVirtualHandStyleProxy_h
#define vtkSMVRVirtualHandStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRTrackStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRVirtualHandStyleProxy
  : public vtkSMVRTrackStyleProxy
{
public:
  static vtkSMVRVirtualHandStyleProxy* New();
  vtkTypeMacro(vtkSMVRVirtualHandStyleProxy, vtkSMVRTrackStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMVRVirtualHandStyleProxy();
  ~vtkSMVRVirtualHandStyleProxy(); // WRS-TODO: no "override" here?  Others have it.

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool CurrentButton;
  bool PrevButton;

  bool EventPress;
  bool EventRelease;

  vtkNew<vtkMatrix4x4> CurrentTrackerMatrix;
  vtkNew<vtkMatrix4x4> InverseTrackerMatrix;
  vtkNew<vtkMatrix4x4> CachedModelMatrix;
  vtkNew<vtkMatrix4x4> NewModelMatrix;

private:
  vtkSMVRVirtualHandStyleProxy(const vtkSMVRVirtualHandStyleProxy&) = delete;
  void operator=(const vtkSMVRVirtualHandStyleProxy&) = delete;
};

#endif // vtkSMVRVirtualHandStyleProxy_h
