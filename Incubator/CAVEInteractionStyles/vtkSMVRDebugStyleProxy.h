// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRDebugStyleProxy
 * @brief   an interaction style to dump out debug information
 *
 * vtkSMVRDebugStyleProxy is an interaction style that binds some button,
 * valuator, and tracker roles, for no other purpose than to dump event
 * information to the terminal.
 */
#ifndef vtkSMVRDebugStyleProxy_h
#define vtkSMVRDebugStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRDebugStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRDebugStyleProxy* New();
  vtkTypeMacro(vtkSMVRDebugStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Overridden to defer expensive calculations and update vtk objects
  bool Update() override;

protected:
  vtkSMVRDebugStyleProxy();
  ~vtkSMVRDebugStyleProxy() override = default;

  void HandleButton(const vtkVREvent& event) override;
  void HandleValuator(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

private:
  vtkSMVRDebugStyleProxy(const vtkSMVRDebugStyleProxy&); // Not implemented
  void operator=(const vtkSMVRDebugStyleProxy&);         // Not implemented

  bool EnableReport;
  vtkNew<vtkMatrix4x4> TrackerMatrix;
};

#endif //  vtkSMVRDebugStyleProxy_h
