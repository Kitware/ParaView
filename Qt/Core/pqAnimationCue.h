// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimationCue_h
#define pqAnimationCue_h

#include "pqProxy.h"

#include "vtkSmartPointer.h"

class vtkSMProxy;
class vtkSMProperty;

/**
 * pqAnimatedPropertyInfo stores information about an animated property.
 */
struct pqAnimatedPropertyInfo
{
  vtkSmartPointer<vtkSMProxy> Proxy;
  QString Name;
  int Index = 0;
};

/**
 * pqAnimationCue is the pqProxy wrapping an animation proxy.
 *
 * It offers a proper API to get informations about the animated vtkSMProxy
 * and to manipulate the cue keyframes.
 * ! Note the difference between the cue proxy (used as constructor parameter)
 * and the animated proxy (the concrete object that will be animated) !
 *
 * Supported cue vtkSMProxy are mainly `CameraAnimationCue`, `PythonAnimationCue`,
 * `TimeAnimationCue` and `KeyFrameAnimationCue`.
 */
class PQCORE_EXPORT pqAnimationCue : public pqProxy
{
  Q_OBJECT
  typedef pqProxy Superclass;

public:
  pqAnimationCue(const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server,
    QObject* parent = nullptr);
  ~pqAnimationCue() override;

  /**
   * Returns the number of keyframes in this cue.
   */
  int getNumberOfKeyFrames() const;

  /**
   * Returns a list of the keyframes.
   */
  QList<vtkSMProxy*> getKeyFrames() const;

  /**
   * Insert a new keyframe at the given index.
   * The time for the key frame is computed using the times
   * for the neighbouring keyframes if any.
   * Returns the newly created keyframe proxy on success,
   * nullptr otherwise.
   */
  vtkSMProxy* insertKeyFrame(int index);

  /**
   * Deletes the keyframe at the given index.
   * Consequently, the keyframesModified() signal will get fired.
   */
  void deleteKeyFrame(int index);

  /**
   * Returns keyframe at a given index, if one exists, nullptr otherwise.
   */
  vtkSMProxy* getKeyFrame(int index) const;

  /**
   * Returns the animated proxy, if any.
   */
  vtkSMProxy* getAnimatedProxy() const;

  /**
   * Returns the property that is animated by this cue, if any.
   */
  vtkSMProperty* getAnimatedProperty() const;

  /**
   * Returns the property name that is animated.
   */
  QString getAnimatedPropertyName() const;

  /**
   * Returns the string version of the camera mode.
   */
  static QString getCameraModeName(int mode);

  /**
   * Returns the index of the property being animated.
   */
  int getAnimatedPropertyIndex() const;

  /**
   * Set the type of the keyframe created by default.
   * default is CompositeKeyFrame.
   */
  void setKeyFrameType(const QString& type) { this->KeyFrameType = type; }

  /**
   * Used by editors to trigger keyframesModified() signal after bulk of
   * modifications have been made to the cue/key frames.
   */
  void triggerKeyFramesModified() { Q_EMIT this->keyframesModified(); }

  ///@{
  /**
   * Get/Set the enabled state for the cue.
   */
  void setEnabled(bool enable);
  bool isEnabled() const;
  ///@}

  /**
   * Return a string formatted from proxy registration name, property name and index.
   */
  QString getDisplayName();

  /**
   * Return true if underlying proxy is of type CameraAnimationCue.
   */
  bool isCameraCue();

  /**
   * Return true if underlying proxy is of type PythonAnimationCue.
   */
  bool isPythonCue();

  /**
   * Return true if underlying proxy is of type TimeAnimationCue.
   */
  bool isTimekeeperCue();

Q_SIGNALS:
  /**
   * emitted when something about the keyframes changes.
   */
  void keyframesModified();

  /**
   * Fired when the animated proxy/property/index changes
   */
  void modified();

  /**
   * Fired when the enabled-state of the cue changes.
   */
  void enabled(bool);

private Q_SLOTS:
  /**
   * Called when the "Enabled" property is changed.
   */
  void onEnabledModified();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqAnimationCue)

  ///@{
  /**
   * Methods used to register/unregister keyframe proxies.
   */
  void addKeyFrameInternal(vtkSMProxy*);
  void removeKeyFrameInternal(vtkSMProxy*);
  QString KeyFrameType;
  ///@}
};
#endif
