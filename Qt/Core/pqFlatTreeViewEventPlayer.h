// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFlatTreeViewEventPlayer_h
#define pqFlatTreeViewEventPlayer_h

#include "pqCoreModule.h"
#include "pqWidgetEventPlayer.h"

/**
Concrete implementation of pqWidgetEventPlayer that translates high-level ParaView events into
low-level Qt events.

\sa pqEventPlayer
*/

class PQCORE_EXPORT pqFlatTreeViewEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqFlatTreeViewEventPlayer(QObject* p = nullptr);

  using Superclass::playEvent;
  bool playEvent(
    QObject* Object, const QString& Command, const QString& Arguments, bool& Error) override;

private:
  pqFlatTreeViewEventPlayer(const pqFlatTreeViewEventPlayer&);
  pqFlatTreeViewEventPlayer& operator=(const pqFlatTreeViewEventPlayer&);
};

#endif // !pqFlatTreeViewEventPlayer_h
