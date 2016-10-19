/*=========================================================================

   Program: ParaView
   Module:    pqCameraDialog.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef pqCameraDialog_h
#define pqCameraDialog_h

#include "pqDialog.h"

class pqCameraDialogInternal;
class pqView;

class PQCOMPONENTS_EXPORT pqCameraDialog : public pqDialog
{
  Q_OBJECT
public:
  pqCameraDialog(QWidget* parent = NULL, Qt::WindowFlags f = 0);
  virtual ~pqCameraDialog();

  void SetCameraGroupsEnabled(bool enabled);

public slots:
  void setRenderModule(pqView*);

private slots:
  // Description:
  // Choose a file and load/save camera properties.
  void saveCameraConfiguration();
  void loadCameraConfiguration();

  // Description:
  // Assign/restore the current camera properties to
  // a custom view button.
  void configureCustomViews();
  void applyCustomView(int buttonId);
  void applyCustomView0() { this->applyCustomView(0); }
  void applyCustomView1() { this->applyCustomView(1); }
  void applyCustomView2() { this->applyCustomView(2); }
  void applyCustomView3() { this->applyCustomView(3); }

  void resetViewDirectionPosX();
  void resetViewDirectionNegX();
  void resetViewDirectionPosY();
  void resetViewDirectionNegY();
  void resetViewDirectionPosZ();
  void resetViewDirectionNegZ();

  void resetViewDirection(
    double look_x, double look_y, double look_z, double up_x, double up_y, double up_z);

  void applyCameraRoll();
  void applyCameraElevation();
  void applyCameraAzimuth();

  void resetRotationCenterWithCamera();

  void setInteractiveViewLinkOpacity(double value);
  void setInteractiveViewLinkBackground(bool hideBackground);
  void updateInteractiveViewLinkWidgets();

protected:
  void setupGUI();

private:
  pqCameraDialogInternal* Internal;

  enum CameraAdjustmentType
  {
    Roll = 0,
    Elevation,
    Azimuth
  };
  void adjustCamera(CameraAdjustmentType enType, double angle);
};

#endif
