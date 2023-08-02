// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqHighlightablePushButton_h
#define pqHighlightablePushButton_h

#include "pqWidgetsModule.h"
#include <QPushButton>
#include <QScopedPointer>

/**
 * pqHighlightablePushButton extents QPushButton to add support for
 * highlighting the button.
 */
class PQWIDGETS_EXPORT pqHighlightablePushButton : public QPushButton
{
  Q_OBJECT
  typedef QPushButton Superclass;

public:
  pqHighlightablePushButton(QWidget* parent = nullptr);
  pqHighlightablePushButton(const QString& text, QWidget* parent = nullptr);
  pqHighlightablePushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);
  ~pqHighlightablePushButton() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Slots to highlight (or clear the highlight).
   */
  void highlight(bool clear = false);
  void clear() { this->highlight(true); }

private:
  Q_DISABLE_COPY(pqHighlightablePushButton)
  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};

#endif
