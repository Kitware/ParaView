// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqKeyFrameEditor_h
#define pqKeyFrameEditor_h

#include "pqComponentsModule.h"
#include <QDialog>
#include <QWidget>

class pqAnimationCue;
class pqAnimationScene;
class QStandardItem;

/**
 * editor for editing animation key frames
 */
class PQCOMPONENTS_EXPORT pqKeyFrameEditor : public QWidget
{
  typedef QWidget Superclass;

  Q_OBJECT

public:
  pqKeyFrameEditor(pqAnimationScene* scene, pqAnimationCue* cue, const QString& label, QWidget* p);
  ~pqKeyFrameEditor() override;

  /**
   * The keyframe editor can be set in a mode where the user can only edit the
   * key frame values or keyframe interpolation and not add/delete keyframes
   * or change key time. To enable this mode, set this to true (false by
   * default).
   */
  void setValuesOnly(bool);
  bool valuesOnly() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * read the key frame data and display it
   */
  void readKeyFrameData();
  /**
   * write the key frame data as edited by the user to the server manager
   */
  void writeKeyFrameData();

Q_SIGNALS:
  void modified();

private Q_SLOTS:
  void newKeyFrame();
  void deleteKeyFrame();
  void deleteAllKeyFrames();

  void updateButtons();

  /**
   * Specific to Camera Cues.
   */
  void useCurrentCamera(QStandardItem* item);
  void useCurrentCameraForSelected();
  void updateCurrentCamera(QStandardItem* item);
  void updateCurrentCameraWithSelected();
  void createOrbitalKeyFrame();
  void importTrajectory();
  void exportTrajectory();
  void updateSplineMode();

private: // NOLINT(readability-redundant-access-specifiers)
  class pqInternal;
  pqInternal* Internal;
};

// internal class
class pqKeyFrameEditorDialog : public QDialog
{
  Q_OBJECT
public:
  pqKeyFrameEditorDialog(QWidget* p, QWidget* child);
  ~pqKeyFrameEditorDialog() override;
  QWidget* Child;
};

#endif
