// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCameraReaction_h
#define pqCameraReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * pqCameraReaction has the logic to handle common operations associated with
 * the camera such as reset view along X axis etc.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCameraReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  enum Mode
  {
    RESET_CAMERA,
    RESET_POSITIVE_X,
    RESET_POSITIVE_Y,
    RESET_POSITIVE_Z,
    RESET_NEGATIVE_X,
    RESET_NEGATIVE_Y,
    RESET_NEGATIVE_Z,
    APPLY_ISOMETRIC_VIEW,
    ZOOM_TO_DATA,
    ROTATE_CAMERA_CW,
    ROTATE_CAMERA_CCW,
    ZOOM_CLOSEST_TO_DATA,
    RESET_CAMERA_CLOSEST
  };

  pqCameraReaction(QAction* parent, Mode mode);

  static void resetCamera(bool closest = false);
  static void resetPositiveX();
  static void resetPositiveY();
  static void resetPositiveZ();
  static void resetNegativeX();
  static void resetNegativeY();
  static void resetNegativeZ();
  static void resetDirection(
    double look_x, double look_y, double look_z, double up_x, double up_y, double up_z);
  static void zoomToData(bool closest = false);
  static void rotateCamera(double angle);
  static void applyIsometricView();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call
   * this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqCameraReaction)
  Mode ReactionMode;
};

#endif
