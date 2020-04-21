/*=========================================================================

   Program: ParaView
   Module:    pqCameraKeyFrameWidget.h

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
#ifndef pqCameraKeyFrameWidget_h
#define pqCameraKeyFrameWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

class vtkSMProxy;
class vtkCamera;

/**
* pqCameraKeyFrameWidget is the widget that is shown to edit the value of a
* single camera key frame. This class is based on pqCameraWidget and hence has
* sections of code borrowed from there.
*/
class PQCOMPONENTS_EXPORT pqCameraKeyFrameWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqCameraKeyFrameWidget(QWidget* parent = 0);
  ~pqCameraKeyFrameWidget() override;

  bool usePathBasedMode() const;

Q_SIGNALS:
  /**
  * Fired when user requests the use of the current camera as the value for
  * the key frame.
  */
  void useCurrentCamera();
  void updateCurrentCamera();

public Q_SLOTS:
  /**
  * Initialize the widget using the values from the key frame proxy.
  */
  void initializeUsingKeyFrame(vtkSMProxy* keyframeProxy);

  /**
  * Initialize the widget using the camera.
  */
  void initializeUsingCamera(vtkCamera* camera);

  /**
   * Initialize the camera using the widget values.
   */
  void applyToCamera(vtkCamera* camera);

  /**
  * The camera keyframes have 2 modes either interpolate vtkCamera's using the
  * camera interpolator or use path-based. This mode is not defined on
  * per-keyframe basis, but on the entire animation track. Hence, we provide
  * this slot to choose which mode should the widget operate in.
  */
  void setUsePathBasedMode(bool);

  /**
  * Write the user chosen values for this key frame to the proxy.
  */
  void saveToKeyFrame(vtkSMProxy* keyframeProxy);

protected:
  // Overridden to update the 3D widget's visibility states.
  void showEvent(QShowEvent*) override;
  void hideEvent(QHideEvent*) override;

private Q_SLOTS:
  /**
  * called when the user clicks on an item in the left pane.
  */
  void changeCurrentPage();

private:
  Q_DISABLE_COPY(pqCameraKeyFrameWidget)

  class pqInternal;
  pqInternal* Internal;
};

#endif
