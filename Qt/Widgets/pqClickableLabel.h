// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqClickableLabel_h
#define pqClickableLabel_h

#include "pqWidgetsModule.h"

#include <QLabel>

/**
 * @brief A simple clickable label that mimics
 * a push button and emits onClicked event
 */
class PQWIDGETS_EXPORT pqClickableLabel : public QLabel
{
  Q_OBJECT

public:
  /**
   * @brief Default constructor is deleted
   */
  pqClickableLabel() = delete;

  /**
   * @brief Default constructor that sets up the tooltip
   * and the label pixmap or text.
   * If the pixmap is nullptr, it will not be set.
   */
  pqClickableLabel(QWidget* widget, const QString& text, const QString& tooltip,
    const QString& statusTip, QPixmap* pixmap, QWidget* parent);

  /**
   * @brief Defaulted destructor for polymorphism
   */
  ~pqClickableLabel() override = default;

Q_SIGNALS:
  /**
   * @brief Signal emitted when the label
   * is clicked (to mimic a push button)
   * @param[in] widget the widget attached to
   * the pqClickableLabel
   */
  void onClicked(QWidget* widget);

protected:
  void mousePressEvent(QMouseEvent* event) override;

  QWidget* Widget = nullptr;
};

#endif
