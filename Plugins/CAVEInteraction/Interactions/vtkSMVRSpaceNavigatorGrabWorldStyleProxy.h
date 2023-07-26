// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRSpaceNavigatorGrabWorldStyleProxy_h
#define vtkSMVRSpaceNavigatorGrabWorldStyleProxy_h

#include "vtkInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkTransform;

struct vtkVREvent;

class VTKINTERACTIONSTYLES_EXPORT vtkSMVRSpaceNavigatorGrabWorldStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRSpaceNavigatorGrabWorldStyleProxy* New();
  vtkTypeMacro(vtkSMVRSpaceNavigatorGrabWorldStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMVRSpaceNavigatorGrabWorldStyleProxy();
  ~vtkSMVRSpaceNavigatorGrabWorldStyleProxy() override;

  void HandleAnalog(const vtkVREvent& event) override;

private:
  vtkSMVRSpaceNavigatorGrabWorldStyleProxy(
    const vtkSMVRSpaceNavigatorGrabWorldStyleProxy&) = delete;              // Not implemented
  void operator=(const vtkSMVRSpaceNavigatorGrabWorldStyleProxy&) = delete; // Not implemented
};

#endif // vtkSMVRSpaceNavigatorGrabWorldStyleProxy_h
