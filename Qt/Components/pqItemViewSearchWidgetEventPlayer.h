// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqItemViewSearchWidgetEventPlayer_h
#define pqItemViewSearchWidgetEventPlayer_h

#include "pqComponentsModule.h" // needed for EXPORT macro.
#include "pqWidgetEventPlayer.h"

/**
 * pqItemViewSearchWidgetEventPlayer is the player for pqItemViewSearchWidget.
 */
class PQCOMPONENTS_EXPORT pqItemViewSearchWidgetEventPlayer : public pqWidgetEventPlayer
{
  Q_OBJECT
  typedef pqWidgetEventPlayer Superclass;

public:
  pqItemViewSearchWidgetEventPlayer(QObject* parent = nullptr);
  ~pqItemViewSearchWidgetEventPlayer() override;

  using Superclass::playEvent;
  bool playEvent(
    QObject* object, const QString& command, const QString& arguments, bool& error) override;

  static const QString& EVENT_NAME();

private:
  Q_DISABLE_COPY(pqItemViewSearchWidgetEventPlayer)
};

#endif
