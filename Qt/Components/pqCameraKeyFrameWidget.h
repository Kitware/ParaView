// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCameraKeyFrameWidget_h
#define pqCameraKeyFrameWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

#include <vtk_jsoncpp_fwd.h> // for forward declarations

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
  pqCameraKeyFrameWidget(QWidget* parent = nullptr);
  ~pqCameraKeyFrameWidget() override;

  /**
   * Return 'true' if the CameraKeyFrameWidget is set to 'path' mode,
   * indicating that the widget's configuration applies to the entire animation
   * track rather than being defined on a per-keyframe basis.
   */
  bool usePathBasedMode() const;

  /**
   * Initialize the widget properties using JSON.
   * Does not work in 'path' mode.
   */
  void initializeUsingJSON(const Json::Value& json);

  /**
   * Generate a JSON representing the widget configuration.
   * Does not work in 'path' mode.
   */
  Json::Value serializeToJSON() const;

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

  /**
   * Set the positions points for path-based keyframe.
   */
  void setPositionPoints(const std::vector<double>&);

  /**
   * Set the focal points for path-based keyframe.
   */
  void setFocalPoints(const std::vector<double>&);

  /**
   * Set the view up for path-based keyframe.
   */
  void setViewUp(double viewUp[3]);

protected:
  // Overridden to update the 3D widget's visibility states.
  void showEvent(QShowEvent*) override;
  void hideEvent(QHideEvent*) override;

private Q_SLOTS:
  /**
   * called when the user clicks on an item in the left pane.
   */
  void changeCurrentPage();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqCameraKeyFrameWidget)

  class pqInternal;
  pqInternal* Internal;
};

#endif
