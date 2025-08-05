// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMVRControlLocationEgoStyleProxy
 * @brief   rotation and translation of a point with a single button
 *
 * vtkSMVRControlLocationEgoStyleProxy is an interaction style that uses the position and
 * orientation of the tracker to control the position of a 3D vector property.
 */
#ifndef vtkSMVRControlLocationEgoStyleProxy_h
#define vtkSMVRControlLocationEgoStyleProxy_h

#include "vtkNew.h"
#include "vtkPVIncubatorCAVEInteractionStylesModule.h" // for export macro
#include "vtkSMVRInteractorStyleProxy.h"

class vtkCamera;
class vtkMatrix4x4;
class vtkSMRenderViewProxy;
class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
struct vtkVREvent;

class VTKPVINCUBATORCAVEINTERACTIONSTYLES_EXPORT vtkSMVRControlLocationEgoStyleProxy
  : public vtkSMVRInteractorStyleProxy
{
public:
  static vtkSMVRControlLocationEgoStyleProxy* New();
  vtkTypeMacro(vtkSMVRControlLocationEgoStyleProxy, vtkSMVRInteractorStyleProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  int GetControlledPropertySize() override { return 3; }

  // Overridden to defer expensive calculations and update vtk objects
  bool Update() override;

protected:
  vtkSMVRControlLocationEgoStyleProxy();
  ~vtkSMVRControlLocationEgoStyleProxy() override = default;

  void HandleButton(const vtkVREvent& event) override;
  void HandleTracker(const vtkVREvent& event) override;

private:
  vtkSMVRControlLocationEgoStyleProxy(
    const vtkSMVRControlLocationEgoStyleProxy&) = delete;              // Not implemented
  void operator=(const vtkSMVRControlLocationEgoStyleProxy&) = delete; // Not implemented

  bool EnableNavigate;    /* mirrors the button assigned the "Navigate World" role */
  bool IsInitialRecorded; /* flag indicating that we're in the middle of a navigation operation */
  double Origin[4];
  double OriginalOrigin[3];
  bool FirstUpdate;

  vtkNew<vtkMatrix4x4> SavedModelMatrix;
  vtkNew<vtkMatrix4x4> SavedInverseModelMatrix;
  vtkNew<vtkMatrix4x4> SavedInverseWandMatrix;
  vtkNew<vtkMatrix4x4> TrackerMatrix;
  vtkNew<vtkMatrix4x4> TransformMatrix;
};

#endif // vtkSMVRControlLocationEgoStyleProxy_h
