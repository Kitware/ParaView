// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRSpaceNavigatorGrabWorldStyleProxy_h
#define vtkSMVRSpaceNavigatorGrabWorldStyleProxy_h

#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkTransform;

struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRSpaceNavigatorGrabWorldStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRSpaceNavigatorGrabWorldStyleProxy* New();
  vtkTypeMacro(vtkSMVRSpaceNavigatorGrabWorldStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMVRSpaceNavigatorGrabWorldStyleProxy();
  ~vtkSMVRSpaceNavigatorGrabWorldStyleProxy() override;

  void HandleValuator(const vtkVREvent& event) override;

private:
  vtkSMVRSpaceNavigatorGrabWorldStyleProxy(
    const vtkSMVRSpaceNavigatorGrabWorldStyleProxy&) = delete;              // Not implemented
  void operator=(const vtkSMVRSpaceNavigatorGrabWorldStyleProxy&) = delete; // Not implemented
};

#endif // vtkSMVRSpaceNavigatorGrabWorldStyleProxy_h
