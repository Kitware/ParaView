// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCoordinateFramePropertyWidget_h
#define pqCoordinateFramePropertyWidget_h

#include "pqInteractivePropertyWidget.h"

/**
 * pqCoordinateFramePropertyWidget is a custom property widget that uses
 * "CoordinateFrameWidgetRepresentation" to help users interactively set the origin
 * and normal for a plane. To use this widget for a property group
 * (vtkSMPropertyGroup), use "InteractiveFrame" as the "panel_widget" in the
 * XML configuration for the proxy. The property group should have properties for
 * following functions:
 * \li \c Origin: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 *        origin of the interactive coordinate frame.
 * \li \c XAxis: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 *        x axis for the interactive coordinate frame.
 * \li \c YAxis: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 *        y axis for the interactive coordinate frame.
 * \li \c ZAxis: a 3-tuple vtkSMDoubleVectorProperty that will be linked to the
 *        z axis for the interactive coordinate frame.
 * \li \c Input: (optional) a vtkSMInputProperty that is used to get data
 *        information for bounds when placing/resetting the widget.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCoordinateFramePropertyWidget
  : public pqInteractivePropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(int lockedAxis READ getLockedAxis WRITE setLockedAxis NOTIFY lockedAxisChangedByUser)
  typedef pqInteractivePropertyWidget Superclass;

public:
  pqCoordinateFramePropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqCoordinateFramePropertyWidget() override;

  /**
   * Set/get which axis is locked. (-1 indicates no axis is locked.)
   */
  int getLockedAxis() const;
  void setLockedAxis(int lockedAxis);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the widget normal to be along the X axis.
   */
  void useXNormal() { this->setNormal(1, 0, 0); }

  /**
   * Set the widget normal to be along the Y axis.
   */
  void useYNormal() { this->setNormal(0, 1, 0); }

  /**
   * Set the widget normal to be along the Z axis.
   */
  void useZNormal() { this->setNormal(0, 0, 1); }

  /**
   * Reset the axes to align with world coordinate axes.
   */
  void resetToWorldXYZ();

  /**
   * Update the widget's origin and bounds using current data bounds.
   */
  void resetToDataBounds();

  /**
   * Reset the camera to be down the locked (or nearest) coordinate-frame axis.
   */
  void resetCameraToNormal();

  /**
   * Set the locked (or nearest) coordinate frame axis to be along the camera view direction.
   */
  void useCameraNormal();

  /**
   * Set the X-,Y-, or Z-axis with user-specified values and update the other axes accordingly.
   */
  void setUserXAxis();
  void setUserYAxis();
  void setUserZAxis();

protected Q_SLOTS:
  /**
   * Places the interactive widget using current data source information.
   */
  void placeWidget() override;

private Q_SLOTS:
  void setOrigin(double x, double y, double z);
  void setNormal(double x, double y, double z);
  void setDirection(double x, double y, double z);
  void currentIndexChangedLockAxis(int);

protected:
  void setUserAxis(int axis);

Q_SIGNALS:
  // Note this reports a lock axis (-1, 0, 1, or 2), not a combo-box index (0, 1, 2, 3)
  void lockedAxisChangedByUser(int);

private:
  Q_DISABLE_COPY(pqCoordinateFramePropertyWidget)
};

#endif
