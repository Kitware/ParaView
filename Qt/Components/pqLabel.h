// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLabel_h
#define pqLabel_h

#include "pqComponentsModule.h"

#include <QLabel>

/**
 * pqLabel is a subclass of QLabel that emits a clicked() signal when
 * the label text is clicked.
 */
class PQCOMPONENTS_EXPORT pqLabel : public QLabel
{
  Q_OBJECT

public:
  pqLabel(const QString& text, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqLabel() override;

  void mousePressEvent(QMouseEvent* event) override;

Q_SIGNALS:
  void clicked();
};

#endif // pqLabel_h
