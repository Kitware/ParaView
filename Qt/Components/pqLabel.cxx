// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLabel.h"

pqLabel::pqLabel(const QString& txt, QWidget* parentObject, Qt::WindowFlags f)
  : QLabel(txt, parentObject, f)
{
}

pqLabel::~pqLabel() = default;

void pqLabel::mousePressEvent(QMouseEvent*)
{
  Q_EMIT clicked();
}
