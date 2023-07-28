// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPluginTreeWidgetEventPlayer_h
#define pqPluginTreeWidgetEventPlayer_h

#include "pqComponentsModule.h"
#include "pqWidgetEventPlayer.h"

/**
Concrete implementation of pqWidgetEventPlayer that translates high-level ParaView events into
low-level Qt events.

\sa pqEventPlayer
*/

class PQCOMPONENTS_EXPORT pqPluginTreeWidgetEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqPluginTreeWidgetEventPlayer(QObject* parent = nullptr);
  ~pqPluginTreeWidgetEventPlayer() override;

  using Superclass::playEvent;
  bool playEvent(
    QObject* object, const QString& command, const QString& arguments, bool& error) override;

private:
  Q_DISABLE_COPY(pqPluginTreeWidgetEventPlayer)
};

#endif // !pqPluginTreeWidgetEventPlayer_h
