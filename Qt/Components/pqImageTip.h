// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqImageTip_h
#define pqImageTip_h

#include "pqComponentsModule.h"
#include <QLabel>
#include <QtGlobal>

class QBasicTimer;
class QPixmap;
class QPoint;

/**
 * Provides tooltip-like behavior, but displays an image instead of text
 */
class PQCOMPONENTS_EXPORT pqImageTip : public QLabel
{
  Q_OBJECT

public:
  /**
   * Displays a pixmap at the given screen coordinates
   */
  static void showTip(const QPixmap& image, const QPoint& pos);

private:
  pqImageTip(const QPixmap& image, QWidget* parent);
  ~pqImageTip() override;

  QBasicTimer* const hideTimer;

  bool eventFilter(QObject*, QEvent*) override;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  void enterEvent(QEvent*) override;
#else
  void enterEvent(QEnterEvent*) override;
#endif
  void timerEvent(QTimerEvent* e) override;
  void paintEvent(QPaintEvent* e) override;
};

#endif
