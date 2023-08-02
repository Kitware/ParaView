// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLoadRestoreWindowLayoutReaction_h
#define pqLoadRestoreWindowLayoutReaction_h

#include "pqReaction.h"

/**
 * @ingroup Reactions
 * Reaction to support saving/restoring main window geometry/layout.
 * This reaction can support both loading and restoring window geometry. Use
 * separate instances of this reaction to each role, passing appropriate value
 * for the \c load argument to the constructor.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLoadRestoreWindowLayoutReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqLoadRestoreWindowLayoutReaction(bool load, QAction* parent = nullptr);
  ~pqLoadRestoreWindowLayoutReaction() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqLoadRestoreWindowLayoutReaction)
  bool Load;
};

#endif
