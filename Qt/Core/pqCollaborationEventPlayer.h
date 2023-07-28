// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCollaborationEventPlayer_h
#define pqCollaborationEventPlayer_h

#include "pqCoreModule.h"
#include "pqWidgetEventPlayer.h"

/**
 * pqCollaborationEventPlayer is used to playback events that make
 * collaborative-testing possible. These events cannot be recorded by the
 * test-recorder, but are manually added. But once added, they enable the
 * playback to wait for appropriate actions to happen.
 */
class PQCORE_EXPORT pqCollaborationEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqCollaborationEventPlayer(QObject* parent = nullptr);
  ~pqCollaborationEventPlayer() override;

  using Superclass::playEvent;
  bool playEvent(
    QObject* object, const QString& command, const QString& arguments, bool& error) override;

  /**
   * used to wait till the process becomes a master.
   */
  static void waitForMaster(int ms = 500);

  /**
   * used to wait until there are num_connection connections.
   */
  static void waitForConnections(int num_connections);

  static void wait(int milli_seconds);

private:
  Q_DISABLE_COPY(pqCollaborationEventPlayer)
};

#endif
