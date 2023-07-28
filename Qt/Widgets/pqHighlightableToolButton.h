// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqHighlightableToolButton_h
#define pqHighlightableToolButton_h

#include "pqWidgetsModule.h"
#include <QPalette>
#include <QToolButton>

/**
 * @class pqHighlightableToolButton
 * @brief QToolButton with ability to highlight the button.
 *
 * pqHighlightableToolButton is similar to pqHighlightablePushButton except that
 * is a subclass of QToolButton  instead of QPushButton.
 */
class PQWIDGETS_EXPORT pqHighlightableToolButton : public QToolButton
{
  Q_OBJECT
  typedef QToolButton Superclass;

public:
  pqHighlightableToolButton(QWidget* parent = nullptr);
  ~pqHighlightableToolButton() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Slots to highlight (or clear the highlight).
   */
  void highlight(bool clear = false);
  void clear() { this->highlight(true); }

private:
  Q_DISABLE_COPY(pqHighlightableToolButton);
  QPalette ResetPalette;
};

#endif
