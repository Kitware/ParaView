// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMVRSkeletonStyleProxy_h
#define vtkSMVRSkeletonStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRSkeletonStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRSkeletonStyleProxy* New();
  vtkTypeMacro(vtkSMVRSkeletonStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMVRSkeletonStyleProxy();
  ~vtkSMVRSkeletonStyleProxy();

  void HandleButton(const vtkVREvent& event) override;
  void HandleValuator(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

  bool EnableReport;

  bool IsInitialTransRecorded;
  bool IsInitialRotRecorded;

  vtkNew<vtkMatrix4x4> InverseInitialTransMatrix;
  vtkNew<vtkMatrix4x4> InverseInitialRotMatrix;

  vtkNew<vtkMatrix4x4> CachedTransMatrix;
  vtkNew<vtkMatrix4x4> CachedRotMatrix;

private:
  vtkSMVRSkeletonStyleProxy(const vtkSMVRSkeletonStyleProxy&); // Not implemented
  void operator=(const vtkSMVRSkeletonStyleProxy&);            // Not implemented
};

#endif //  vtkSMVRSkeletonStyleProxy_h
