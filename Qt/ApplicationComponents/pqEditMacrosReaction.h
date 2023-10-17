// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqEditMacrosReaction_h
#define pqEditMacrosReaction_h

#include "pqMasterOnlyReaction.h"

class QAction;
class pqEditMacrosDialog;

/**
 * @ingroup Reactions
 * pqEditMacrosReaction is the reaction to pop-up the edit macros dialog.
 *
 * @warning This class is built only when Python is enabled.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqEditMacrosReaction : public pqMasterOnlyReaction
{
  Q_OBJECT
  typedef pqMasterOnlyReaction Superclass;

public:
  pqEditMacrosReaction(QAction* action);

  /**
   * Pops-up the pqEditMacrosDialog dialog.
   */
  static void configureMacros();

protected:
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqEditMacrosReaction)

  static QPointer<pqEditMacrosDialog> Dialog;
};

#endif
