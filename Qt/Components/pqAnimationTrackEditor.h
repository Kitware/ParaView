// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimationTrackEditor_h
#define pqAnimationTrackEditor_h

#include "pqComponentsModule.h"

#include <QObject>
#include <memory> // for unique_ptr

class pqAnimationCue;
class pqAnimationScene;

/**
 * pqAnimationTrackEditor holds the main dialog to edit animation tracks.
 * The content of the dialog depends on the cue type (python, timekeeper,
 * camera, or other property)
 */
class PQCOMPONENTS_EXPORT pqAnimationTrackEditor : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqAnimationTrackEditor(pqAnimationScene* scene, pqAnimationCue* parent = nullptr);
  ~pqAnimationTrackEditor() override;

  /**
   * Raise the editor dialog.
   */
  void showEditor();

private:
  Q_DISABLE_COPY(pqAnimationTrackEditor)
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};
#endif
