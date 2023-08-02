// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCustomViewpointsDefaultController_h
#define pqCustomViewpointsDefaultController_h

#include "pqApplicationComponentsModule.h"

#include "pqCustomViewpointsController.h"

/**
 * @brief Default custom viewpoints controller
 *
 * This controller controls the global desktop view custom viewpointss
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCustomViewpointsDefaultController
  : public pqCustomViewpointsController
{
  Q_OBJECT
  typedef pqCustomViewpointsController Superclass;

public:
  pqCustomViewpointsDefaultController(QObject* parent = nullptr);

  ~pqCustomViewpointsDefaultController() override = default;

  /**
   * @brief Get tooltips of all viewpoints
   * @return QStringList of tooltips
   * @see pqCameraDialog::CustomViewpointToolTips
   */
  QStringList getCustomViewpointToolTips() override;

  /**
   * @brief Called when configure button is pressed
   * @see pqCameraDialog::configureCustomViewpoints
   */
  void configureCustomViewpoints() override;

  /**
   * @brief Set the specified viewpoint entry to current viewpoint
   * @see pqCameraDialog::setToCurrentViewpoint
   */
  void setToCurrentViewpoint(int index) override;

  /**
   * @brief Move camera to match specified viewpoint entry
   * @see pqCameraDialog::applyCustomViewpoint
   */
  void applyCustomViewpoint(int index) override;

  /**
   * @brief Remove a custom viewpoint entry
   * @see pqCameraDialog::deleteCustomViewpoint
   */
  void deleteCustomViewpoint(int index) override;

  /**
   * @brief Save current viewpoint in a new viewpoint entry
   * @see pqCameraDialog::addCurrentViewpointToCustomViewpoints
   */
  void addCurrentViewpointToCustomViewpoints() override;
};

#endif
