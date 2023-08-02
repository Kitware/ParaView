// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqScaleByButton_h
#define pqScaleByButton_h

#include "pqWidgetsModule.h"
#include <QMap> // needed for QMap
#include <QToolButton>

/**
 * @class pqScaleByButton
 * @brief Custom QToolButton that adds a menu to key user scale by a factor.
 *
 * This is simply a QToolButton with a menu. The menu has actions which
 * correspond to scale factors. When user clicks any of those actions,
 * `pqScaleByButton::scale` signal is fired with the argument as the factor.
 */
class PQWIDGETS_EXPORT pqScaleByButton : public QToolButton
{
  Q_OBJECT
  typedef QToolButton Superclass;

public:
  /**
   * Creates the button with default scale factors or `0.5X` and `2.0X`.
   */
  pqScaleByButton(QWidget* parent = nullptr);

  /**
   * Creates the button with specified scale factors. The label for the actions
   * is created by use the suffix specified.
   */
  pqScaleByButton(
    const QList<double>& scaleFactors, const QString& suffix = "X", QWidget* parent = nullptr);

  /**
   * Creates the button with specified scale factors and labels.
   */
  pqScaleByButton(const QMap<double, QString>& scaleFactorsAndLabels, QWidget* parent = nullptr);

  ~pqScaleByButton() override;

Q_SIGNALS:
  /**
   * Fired when the action corresponding to a scale factor is triggered.
   */
  void scale(double factor);

private Q_SLOTS:
  void scaleTriggered();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqScaleByButton);
  void constructor(const QMap<double, QString>& scaleFactors);
};

#endif
