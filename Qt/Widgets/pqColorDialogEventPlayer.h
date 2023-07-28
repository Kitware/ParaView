// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqColorDialogEventPlayer_h
#define pqColorDialogEventPlayer_h

#include "pqWidgetEventPlayer.h"
#include "pqWidgetsModule.h" // needed for EXPORT macro.

/**
 * pqColorDialogEventPlayer is the player for QColorDialog. Events are recorded
 * using pqColorDialogEventTranslator.
 */
class PQWIDGETS_EXPORT pqColorDialogEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqColorDialogEventPlayer(QObject* parent = nullptr);
  ~pqColorDialogEventPlayer() override;

  using Superclass::playEvent;
  bool playEvent(
    QObject* object, const QString& command, const QString& arguments, bool& error) override;

  static const QString& EVENT_NAME();

private:
  Q_DISABLE_COPY(pqColorDialogEventPlayer)
};

#endif
