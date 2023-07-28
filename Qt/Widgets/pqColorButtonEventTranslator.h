// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqColorButtonEventTranslator_h
#define pqColorButtonEventTranslator_h

#include "pqWidgetEventTranslator.h"
#include "pqWidgetsModule.h" // needed for EXPORT macro.
#include <QColor>

/**
 * pqColorButtonEventTranslator translates events on pqColorChooserButton
 * or subclass so that they can be recorded in tests in a platform independent
 * way.
 */
class PQWIDGETS_EXPORT pqColorButtonEventTranslator : public pqWidgetEventTranslator
{
  Q_OBJECT
  typedef pqWidgetEventTranslator Superclass;

public:
  pqColorButtonEventTranslator(QObject* parent = nullptr);
  ~pqColorButtonEventTranslator() override;

  /**
   * Overridden to handle events on QColorDialog.
   */
  using Superclass::translateEvent;
  bool translateEvent(QObject* Object, QEvent* Event, bool& Error) override;

private Q_SLOTS:
  void onColorChosen(const QColor&);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqColorButtonEventTranslator)
};

#endif
