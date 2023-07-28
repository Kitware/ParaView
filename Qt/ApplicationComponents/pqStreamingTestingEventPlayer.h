// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqStreamingTestingEventPlayer_h
#define pqStreamingTestingEventPlayer_h

#include "pqApplicationComponentsModule.h"
#include "pqWidgetEventPlayer.h"
#include <QPointer>

class pqViewStreamingBehavior;
/**
Concrete implementation of pqWidgetEventPlayer that handles playback of recorded
pqViewStreamingBehavior.

\sa pqEventPlayer
*/

class PQAPPLICATIONCOMPONENTS_EXPORT pqStreamingTestingEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqStreamingTestingEventPlayer(QObject* p)
    : Superclass(p)
  {
  }
  using Superclass::playEvent;
  bool playEvent(QObject*, const QString& command, const QString& arguments, bool& error) override;

  void setViewStreamingBehavior(pqViewStreamingBehavior*);
  pqViewStreamingBehavior* viewStreamingBehavior();

protected:
  QPointer<pqViewStreamingBehavior> StreamingBehavior;
};

#endif
