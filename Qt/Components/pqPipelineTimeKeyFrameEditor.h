// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPipelineTimeKeyFrameEditor_h
#define pqPipelineTimeKeyFrameEditor_h

#include "pqComponentsModule.h"
#include <QDialog>

class pqAnimationScene;
class pqAnimationCue;

/**
 * editor for editing pipeline time key frames
 */
class PQCOMPONENTS_EXPORT pqPipelineTimeKeyFrameEditor : public QDialog
{
  typedef QDialog Superclass;
  Q_OBJECT
public:
  pqPipelineTimeKeyFrameEditor(pqAnimationScene* scene, pqAnimationCue* cue, QWidget* p);
  ~pqPipelineTimeKeyFrameEditor() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * read the key frame data and display it
   */
  void readKeyFrameData();
  /**
   * write the key frame data as edited by the user to the server manager
   */
  void writeKeyFrameData();

protected Q_SLOTS:
  void updateState();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
