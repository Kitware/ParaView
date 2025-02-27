// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCameraDialog_h
#define pqCameraDialog_h

#include "pqDialog.h"

class pqCameraDialogInternal;
class pqView;
class vtkSMRenderViewProxy;
class QDoubleSpinBox;

class PQCOMPONENTS_EXPORT pqCameraDialog : public pqDialog
{
  Q_OBJECT
public:
  pqCameraDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqCameraDialog() override;

  void SetCameraGroupsEnabled(bool enabled);

  /**
   * Open the CustomViewpointDialog to configure customViewpoints
   */
  static bool configureCustomViewpoints(QWidget* parentWidget, vtkSMRenderViewProxy* viewProxy);

  /**
   * Add the current viewpoint to the custom viewpoints
   */
  static bool addCurrentViewpointToCustomViewpoints(vtkSMRenderViewProxy* viewProxy);

  /**
   * Change cemara positing to an indexed custom viewpoints
   */
  static bool applyCustomViewpoint(int CustomViewpointIndex, vtkSMRenderViewProxy* viewProxy);

  /**
   * Delete an indexed custom viewpoint
   */
  static bool deleteCustomViewpoint(int CustomViewpointIndex, vtkSMRenderViewProxy* viewProxy);

  /**
   * Set an indexed custom viewpoint to the current viewpoint
   */
  static bool setToCurrentViewpoint(int CustomViewpointIndex, vtkSMRenderViewProxy* viewProxy);

  /**
   * Return the list of custom viewpoints tooltups
   */
  static QStringList CustomViewpointToolTips();

  /**
   * Return the list of custom viewpoint configurations
   */
  static QStringList CustomViewpointConfigurations();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setRenderModule(pqView*);

private Q_SLOTS:
  // Description:
  // Choose a file and load/save camera properties.
  void saveCameraConfiguration();
  void loadCameraConfiguration();

  // Description:
  // Assign/restore the current camera properties to
  // a custom view button.
  void configureCustomViewpoints();
  void applyCustomViewpoint();
  void addCurrentViewpointToCustomViewpoints();
  void updateCustomViewpointButtons();

  void resetViewDirectionPosX();
  void resetViewDirectionNegX();
  void resetViewDirectionPosY();
  void resetViewDirectionNegY();
  void resetViewDirectionPosZ();
  void resetViewDirectionNegZ();

  void resetViewDirection(
    double look_x, double look_y, double look_z, double up_x, double up_y, double up_z);

  void applyIsometricView();
  void applyCameraRollPlus();
  void applyCameraRollMinus();
  void applyCameraElevationPlus();
  void applyCameraElevationMinus();
  void applyCameraAzimuthPlus();
  void applyCameraAzimuthMinus();
  void applyCameraZoomIn();
  void applyCameraZoomOut();

  void resetRotationCenterWithCamera();

  void setInteractiveViewLinkOpacity(double value);
  void setInteractiveViewLinkBackground(bool hideBackground);
  void updateInteractiveViewLinkWidgets();

protected:
  void setupGUI();

private:
  pqCameraDialogInternal* Internal;
};

#endif
