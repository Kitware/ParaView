// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRepresentationToolbar_h
#define pqRepresentationToolbar_h

#include "pqApplicationComponentsModule.h"
#include <QToolBar>

/**
 * pqRepresentationToolbar is the toolbar which allows the user to choose the
 * representation type for the active representation.
 * Uses pqDisplayRepresentationWidget internally.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqRepresentationToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqRepresentationToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  pqRepresentationToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }

private:
  Q_DISABLE_COPY(pqRepresentationToolbar)
  void constructor();
};

#endif
