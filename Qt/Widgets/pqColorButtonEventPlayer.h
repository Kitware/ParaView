// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqColorButtonEventPlayer_h
#define pqColorButtonEventPlayer_h

#include "pqWidgetEventPlayer.h"
#include "pqWidgetsModule.h" // needed for EXPORT macro.

/**
 * pqColorButtonEventPlayer is the player for pqColorChooserButton.
 */
class PQWIDGETS_EXPORT pqColorButtonEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqColorButtonEventPlayer(QObject* parent = nullptr);
  ~pqColorButtonEventPlayer() override;

  using Superclass::playEvent;
  bool playEvent(
    QObject* object, const QString& command, const QString& arguments, bool& error) override;

  static const QString& EVENT_NAME();

private:
  Q_DISABLE_COPY(pqColorButtonEventPlayer)
};

#endif
