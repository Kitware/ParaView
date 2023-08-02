// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqColorToolbar_h
#define pqColorToolbar_h

#include "pqApplicationComponentsModule.h"
#include <QToolBar>

/**
 * pqColorToolbar is the toolbar that allows the user to choose the scalar
 * color or solid color for the active representation.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqColorToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  pqColorToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }

private:
  Q_DISABLE_COPY(pqColorToolbar)

  void constructor();
};

#endif
