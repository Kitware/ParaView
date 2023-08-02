// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqCustomViewpointsController_h
#define pqCustomViewpointsController_h

#include "pqApplicationComponentsModule.h"

#include <QObject>

class pqCustomViewpointsToolbar;

/**
 * @brief Base class for custom viewpoints controllers
 *
 * pqCustomViewpointsController is an abstract class that controls
 * the behaviour of a pqCustomViewpointsToolbar
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCustomViewpointsController : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCustomViewpointsController(QObject* parent = nullptr)
    : Superclass(parent)
  {
  }

  ~pqCustomViewpointsController() override = default;

  /**
   * @brief Set the toolbar controlled by `this`
   *
   * It will set `toolbar` as parent of `this`
   */
  void setToolbar(pqCustomViewpointsToolbar* toolbar);

  /**
   * @brief Get the toolbar controlled by `this`
   */
  pqCustomViewpointsToolbar* getToolbar() const { return this->Toolbar; }

  /**
   * @brief Get tooltips of all viewpoints
   * @return QStringList of tooltips
   */
  virtual QStringList getCustomViewpointToolTips() = 0;

  /**
   * @brief Called when configure button is pressed
   */
  virtual void configureCustomViewpoints() = 0;

  /**
   * @brief Set the specified viewpoint entry to current viewpoint
   */
  virtual void setToCurrentViewpoint(int index) = 0;

  /**
   * @brief Move camera to match specified viewpoint entry
   */
  virtual void applyCustomViewpoint(int index) = 0;

  /**
   * @brief Remove a custom viewpoint entry
   */
  virtual void deleteCustomViewpoint(int index) = 0;

  /**
   * @brief Save current viewpoint in a new viewpoint entry
   */
  virtual void addCurrentViewpointToCustomViewpoints() = 0;

private:
  pqCustomViewpointsToolbar* Toolbar = nullptr;
};

#endif
