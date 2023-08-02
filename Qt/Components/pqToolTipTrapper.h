// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqToolTipTrapper_h
#define pqToolTipTrapper_h

#include "pqComponentsModule.h"

#include <QObject>

/**
 * To prevent tooltips from appearing, create an instance of this object.
 */
class PQCOMPONENTS_EXPORT pqToolTipTrapper : public QObject
{
  Q_OBJECT

public:
  pqToolTipTrapper();
  ~pqToolTipTrapper() override;

  bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif
