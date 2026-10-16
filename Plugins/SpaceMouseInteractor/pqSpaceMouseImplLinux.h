// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSpaceMouseImplLinux_h
#define pqSpaceMouseImplLinux_h

#include "pqTimer.h"
#include <QObject>
#include <QPointer>

#include <array>

class pqRenderView;
class pqView;
class QSocketNotifier;

/**
 * Receive spacenavd motion on the GUI thread and navigate the active Render View.
 * Coalesce device events into frames and discard motion while the server is busy.
 */
class pqSpaceMouseImpl : public QObject
{
  Q_OBJECT

public:
  pqSpaceMouseImpl();
  ~pqSpaceMouseImpl() override;

public Q_SLOTS:
  void setActiveView(pqView* view);

private:
  void connectToDaemon();
  void disconnectFromDaemon();
  void readEvents();
  void flushMotion();
  void moveCamera(const std::array<double, 6>& motion, double dt);
  void clearMotion();
  bool isServerBusy(pqRenderView* view) const;
  void startInteraction();
  bool endInteraction();
  void render();

  QPointer<pqRenderView> View;
  // The interaction may still need to end after its view stops being active.
  QPointer<pqRenderView> InteractionView;
  QSocketNotifier* Notifier = nullptr;
  pqTimer ReconnectTimer;
  pqTimer FrameTimer;
  pqTimer IdleTimer;
  pqTimer EndTimer;
  std::array<double, 6> PendingMotion = {};
  unsigned int PendingDurationMs = 0;
  bool HasPendingMotion = false;
  bool StopAfterFrame = false;
  bool Interacting = false;
};

#endif
