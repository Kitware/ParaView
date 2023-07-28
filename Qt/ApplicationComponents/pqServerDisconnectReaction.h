// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqServerDisconnectReaction_h
#define pqServerDisconnectReaction_h

#include "pqReaction.h"
#include "pqTimer.h" // needed for pqTimer.

/**
 * @ingroup Reactions
 * Reaction to disconnect from a server.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqServerDisconnectReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqServerDisconnectReaction(QAction* parent);

  /**
   * Disconnects from active server.
   * Note that this method is static. Applications can simply use this without
   * having to create a reaction instance.
   */
  static void disconnectFromServer();

  /**
   * Disconnects from active server with a warning message to the user to
   * confirm that active proxies will be destroyed, if any. Returns true if
   * disconnect happened, false if not i.e. the user cancelled the operation.
   */
  static bool disconnectFromServerWithWarning();
private Q_SLOTS:
  void updateState();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

  pqTimer UpdateTimer;

private:
  Q_DISABLE_COPY(pqServerDisconnectReaction)
};

#endif
