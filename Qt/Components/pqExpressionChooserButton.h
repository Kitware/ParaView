// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqExpressionChooserButton_h
#define pqExpressionChooserButton_h

#include "pqComponentsModule.h"
#include <QToolButton>

/**
 * @class pqExpressionChooserButton
 * @brief Custom QToolButton that adds a menu to select expression.
 *
 * This is simply a QToolButton with a menu. The menu has actions which
 * correspond to expressions. When user clicks any of those actions,
 * `pqExpressionChooserButton::expressionSelected` signal is fired with the
 * expression as argument.
 */
class PQCOMPONENTS_EXPORT pqExpressionChooserButton : public QToolButton
{
  Q_OBJECT
  typedef QToolButton Superclass;

public:
  pqExpressionChooserButton(QWidget* parent, const QString& group = "");
  ~pqExpressionChooserButton() override;

  void setGroup(const QString& group);

Q_SIGNALS:
  void expressionSelected(const QString& expr) const;

protected Q_SLOTS:
  void updateMenu();

protected: // NOLINT(readability-redundant-access-specifiers)
  QString Group;

private:
  Q_DISABLE_COPY(pqExpressionChooserButton);
};

#endif
