// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqColorDialogEventTranslator_h
#define pqColorDialogEventTranslator_h

#include "pqWidgetEventTranslator.h"
#include "pqWidgetsModule.h" // needed for EXPORT macro.
#include <QColor>

/**
 * pqColorDialogEventTranslator translates events on QColorDialog that they can
 * be recorded in tests in a platform independent way.
 */
class PQWIDGETS_EXPORT pqColorDialogEventTranslator : public pqWidgetEventTranslator
{
  Q_OBJECT
  typedef pqWidgetEventTranslator Superclass;

public:
  pqColorDialogEventTranslator(QObject* parent = nullptr);
  ~pqColorDialogEventTranslator() override;

  /**
   * Overridden to handle events on QColorDialog.
   */
  using Superclass::translateEvent;
  bool translateEvent(QObject* Object, QEvent* Event, bool& Error) override;

private Q_SLOTS:
  void onColorChosen(const QColor&);
  void onFinished(int);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqColorDialogEventTranslator)
};

#endif
