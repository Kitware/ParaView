// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqUndoRedoBehavior_h
#define pqUndoRedoBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

/**
 * @ingroup Behaviors
 * pqUndoRedoBehavior enables application wide undo-redo.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqUndoRedoBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqUndoRedoBehavior(QObject* parent = nullptr);

protected:
private:
  Q_DISABLE_COPY(pqUndoRedoBehavior)
};

#endif
