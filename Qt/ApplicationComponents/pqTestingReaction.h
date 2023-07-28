// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTestingReaction_h
#define pqTestingReaction_h

#include "pqMasterOnlyReaction.h"

/**
 * @ingroup Reactions
 * pqTestingReaction can be used to recording or playing back tests.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTestingReaction : public pqMasterOnlyReaction
{
  Q_OBJECT
  typedef pqMasterOnlyReaction Superclass;

public:
  enum Mode
  {
    RECORD,
    PLAYBACK,
    LOCK_VIEW_SIZE,
    LOCK_VIEW_SIZE_CUSTOM
  };

  pqTestingReaction(QAction* parentObject, Mode mode, Qt::ConnectionType type = Qt::AutoConnection);

  /**
   * Records test.
   */
  static void recordTest(const QString& filename);
  static void recordTest();

  /**
   * Plays test.
   */
  static void playTest(const QString& filename);
  static void playTest();

  /**
   * Locks the view size for testing.
   */
  static void lockViewSize(bool);

  /**
   * Locks the view size with a custom resolution.
   */
  static void lockViewSizeCustom();

protected:
  void onTriggered() override
  {
    switch (this->ReactionMode)
    {
      case RECORD:
        pqTestingReaction::recordTest();
        break;
      case PLAYBACK:
        pqTestingReaction::playTest();
        break;
      case LOCK_VIEW_SIZE:
        pqTestingReaction::lockViewSize(this->parentAction()->isChecked());
        break;
      case LOCK_VIEW_SIZE_CUSTOM:
        pqTestingReaction::lockViewSizeCustom();
        break;
    }
  }

private:
  Q_DISABLE_COPY(pqTestingReaction)
  Mode ReactionMode;
};

#endif
