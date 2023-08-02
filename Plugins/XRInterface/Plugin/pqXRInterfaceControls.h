// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqXRInterfaceControls
 * @brief   ParaView GUI for use within virtual reality
 *
 * This class brings elements of the ParaView GUI into VR where they
 * can be used. Instantiated by the pqXRInterfaceDockPanel.
 */

#ifndef pqXRInterfaceControls_h
#define pqXRInterfaceControls_h

#include "vtkPVXRInterfaceHelper.h"
#include "vtkVRInteractorStyle.h"

#include <QScopedPointer>
#include <QStringList>
#include <QWidget>

class pqPipelineSource;
class pqVCRController;

class pqXRInterfaceControls : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqXRInterfaceControls(vtkPVXRInterfaceHelper* val, QWidget* p = nullptr);
  ~pqXRInterfaceControls() override;

  pqPipelineSource* GetSelectedPipelineSource();

  /**
   * Set the value of the Right Trigger combobox.
   */
  void SetRightTriggerMode(vtkPVXRInterfaceHelper::RightTriggerAction action);

  /**
   * Set the value of the Movement Style combobox.
   */
  void SetMovementStyle(vtkVRInteractorStyle::MovementStyle style);

  /**
   * Set the value of the Motion Factor combobox.
   */
  void SetCurrentMotionFactor(double val);

  /**
   * Set the value of the View Up combobox.
   */
  void SetCurrentViewUp(std::string dir);

  /**
   * Set check state of the Show Floor checkbox.
   */
  void SetShowFloor(bool checked);

  /**
   * Set check state of the Interactive Ray checkbox.
   */
  void SetInteractiveRay(bool checked);

  /**
   * Set check state of the Navigation Panel checkbox.
   */
  void SetNavigationPanel(bool checked);

  /**
   * Set check state of the Snap Crop Planes checkbox.
   */
  void SetSnapCropPlanes(bool checked);

  /**
   * Update custom viewpoints toolbar.
   *
   * Useful when helper modifies locations directly (e.g state loading)
   */
  void UpdateCustomViewpointsToolbar();

protected Q_SLOTS:
  void resetCamera();
  void resetPositions();

private:
  void constructor(vtkPVXRInterfaceHelper* val);

  struct pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
