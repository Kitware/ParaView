// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSignalsBlocker_h
#define pqSignalsBlocker_h

#include "pqCoreModule.h"
#include <QObject>

/**
 * pqSignalsBlocker only exposes a custom signal to act as an intermediate object on which to call
 * blockSignals(). This is for example useful in conjonction with vtkEventQtSlotConnect to be able
 * to block the emulated signal of the vtkObject.
 */
class PQCORE_EXPORT pqSignalsBlocker : public QObject
{
  Q_OBJECT
public:
  pqSignalsBlocker(QObject* parent = nullptr);

Q_SIGNALS:
  void passSignal();
};

#endif
