/*=========================================================================

   Program: ParaView
   Module:    pqCameraReaction.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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

public Q_SLOTS:
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
