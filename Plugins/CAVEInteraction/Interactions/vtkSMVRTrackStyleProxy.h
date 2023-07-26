// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRTrackStyleProxy_h
#define vtkSMVRTrackStyleProxy_h

#include "vtkInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkTransform;

struct vtkVREvent;

class VTKINTERACTIONSTYLES_EXPORT vtkSMVRTrackStyleProxy : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRTrackStyleProxy* New();
  vtkTypeMacro(vtkSMVRTrackStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetControlledPropertySize() override { return 16; }

protected:
  vtkSMVRTrackStyleProxy();
  ~vtkSMVRTrackStyleProxy() override;
  void HandleTracker(const vtkVREvent& event) override;

private:
  vtkSMVRTrackStyleProxy(const vtkSMVRTrackStyleProxy&) = delete; // Not implemented.
  void operator=(const vtkSMVRTrackStyleProxy&) = delete;         // Not implemented.
};

#endif // vtkSMVRTrackStyleProxy_h
