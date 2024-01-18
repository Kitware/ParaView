// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqExpressionsWidget_h
#define pqExpressionsWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqOneLinerTextEdit;

/**
 * pqExpressionsWidget is a widget to edit expression.
 *
 * It is a container for a line edit and buttons linked to the ExpressionsManager.
 */
class PQCOMPONENTS_EXPORT pqExpressionsWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqExpressionsWidget(QWidget* parent = nullptr, const QString& groupName = "");
  ~pqExpressionsWidget() override = default;

  /**
   * Get the internal line edit.
   */
  pqOneLinerTextEdit* lineEdit();

  /**
   * Set buttons up for "groupName" expressions group.
   */
  void setupButtons(const QString& groupName);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Clear the expression from the line edit.
   */
  void clear();

private:
  Q_DISABLE_COPY(pqExpressionsWidget)

  pqOneLinerTextEdit* OneLiner;
};

#endif
