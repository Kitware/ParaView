// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqXRCustomViewpointsController_h
#define pqXRCustomViewpointsController_h

#include "pqApplicationComponentsModule.h"

#include "pqCustomViewpointsController.h"

class pqCustomViewpointsToolbar;
class vtkPVXRInterfaceHelper;

/**
 * @brief Controller for XR custom viewpoints
 *
 * pqXRCustomViewpointsController is bound to a vtkPVXRInterfaceHelper that contains
 * the context of the XR instance.
 */
class pqXRCustomViewpointsController : public pqCustomViewpointsController
{
  Q_OBJECT
  typedef pqCustomViewpointsController Superclass;

public:
  pqXRCustomViewpointsController(vtkPVXRInterfaceHelper* helper, QObject* parent = nullptr);

  ~pqXRCustomViewpointsController() override = default;

  /**
   * @brief Get tooltips of all viewpoints
   * @return QStringList of tooltips
   * @see vtkPVXRInterfaceHelper::GetCustomViewpointToolTips
   */
  QStringList getCustomViewpointToolTips() override;

  /**
   * @brief Called when configure button is pressed
   *
   * This function clears all viewpoints since there is no config menu in XR
   *
   * @see vtkPVXRInterfaceHelper::ClearCameraPoses
   */
  void configureCustomViewpoints() override;

  /**
   * @brief Set the specified viewpoint entry to current viewpoint
   * @see vtkPVXRInterfaceHelper::SaveCameraPose
   */
  void setToCurrentViewpoint(int index) override;

  /**
   * @brief Move camera to match specified viewpoint entry
   * @see vtkPVXRInterfaceHelper::LoadCameraPose
   */
  void applyCustomViewpoint(int index) override;

  /**
   * @brief Unsupported: Remove a custom viewpoint entry
   *
   * This function does nothing since there is no config menu in XR
   */
  void deleteCustomViewpoint(int index) override;

  /**
   * @brief Save current viewpoint in a new viewpoint entry
   * @see vtkPVXRInterfaceHelper::SaveCameraPose
   */
  void addCurrentViewpointToCustomViewpoints() override;

private:
  vtkPVXRInterfaceHelper* Helper = nullptr;
  std::size_t ViewpointCount = 0;
};

#endif
