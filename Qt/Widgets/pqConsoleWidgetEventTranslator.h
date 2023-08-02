// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqConsoleWidgetEventTranslator_h
#define pqConsoleWidgetEventTranslator_h

#include "pqWidgetEventTranslator.h"
#include "pqWidgetsModule.h" // needed for EXPORT macro.
#include <QPointer>          // needed for QPointer

class pqConsoleWidget;

/**
 * pqConsoleWidgetEventTranslator is used to record events from pqConsoleWidget
 * testing.
 */
class PQWIDGETS_EXPORT pqConsoleWidgetEventTranslator : public pqWidgetEventTranslator
{
  Q_OBJECT
  typedef pqWidgetEventTranslator Superclass;

public:
  pqConsoleWidgetEventTranslator(QObject* parent = nullptr);
  ~pqConsoleWidgetEventTranslator() override;

  /**
   * Translate the event, if possible.
   */
  using Superclass::translateEvent;
  bool translateEvent(QObject* target, QEvent* qtevent, bool& errorFlag) override;

protected Q_SLOTS:
  void recordCommand(const QString& text);

private:
  Q_DISABLE_COPY(pqConsoleWidgetEventTranslator)

  QPointer<pqConsoleWidget> CurrentObject;
};

#endif
