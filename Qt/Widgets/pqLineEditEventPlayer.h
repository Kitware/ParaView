// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLineEditEventPlayer_h
#define pqLineEditEventPlayer_h

#include "pqAbstractStringEventPlayer.h"
#include "pqWidgetsModule.h"

/**
 * pqLineEditEventPlayer extends pqAbstractStringEventPlayer to ensure that
 * pqLineEdit fires textChangedAndEditingFinished() signals in
 * playback when "set_string" is handled.
 */
class PQWIDGETS_EXPORT pqLineEditEventPlayer : public pqAbstractStringEventPlayer
{
  Q_OBJECT
  typedef pqAbstractStringEventPlayer Superclass;

public:
  pqLineEditEventPlayer(QObject* parent = nullptr);
  ~pqLineEditEventPlayer() override;

  using Superclass::playEvent;
  bool playEvent(
    QObject* Object, const QString& Command, const QString& Arguments, bool& Error) override;

private:
  Q_DISABLE_COPY(pqLineEditEventPlayer)
};

#endif
