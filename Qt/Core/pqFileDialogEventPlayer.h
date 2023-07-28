// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFileDialogEventPlayer_h
#define pqFileDialogEventPlayer_h

#include "pqCoreModule.h"
#include <pqWidgetEventPlayer.h>

/**
Concrete implementation of pqWidgetEventPlayer that handles playback of recorded file dialog user
input.

\sa pqEventPlayer
*/
class PQCORE_EXPORT pqFileDialogEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqFileDialogEventPlayer(QObject* p = nullptr);

  using Superclass::playEvent;
  bool playEvent(
    QObject* Object, const QString& Command, const QString& Arguments, bool& Error) override;

private:
  pqFileDialogEventPlayer(const pqFileDialogEventPlayer&);
  pqFileDialogEventPlayer& operator=(const pqFileDialogEventPlayer&);
};

#endif // !pqFileDialogEventPlayer_h
