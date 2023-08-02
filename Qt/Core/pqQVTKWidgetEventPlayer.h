// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqQVTKWidgetEventPlayer_h
#define pqQVTKWidgetEventPlayer_h

#include "pqCoreModule.h"
#include "pqWidgetEventPlayer.h"

/**
Concrete implementation of pqWidgetEventPlayer that handles playback of "activate" events for
buttons and menus.

\sa pqEventPlayer
*/
class PQCORE_EXPORT pqQVTKWidgetEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqQVTKWidgetEventPlayer(QObject* p = nullptr);

  using Superclass::playEvent;
  bool playEvent(
    QObject* Object, const QString& Command, const QString& Arguments, bool& Error) override;

private:
  pqQVTKWidgetEventPlayer(const pqQVTKWidgetEventPlayer&);
  pqQVTKWidgetEventPlayer& operator=(const pqQVTKWidgetEventPlayer&);
};

#endif // !pqQVTKWidgetEventPlayer_h
