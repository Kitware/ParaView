// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPluginTreeWidget_h
#define pqPluginTreeWidget_h

#include <QTreeWidget>

#include "pqComponentsModule.h"

class PQCOMPONENTS_EXPORT pqPluginTreeWidget : public QTreeWidget
{
  typedef QTreeWidget Superclass;
  Q_OBJECT

public:
  pqPluginTreeWidget(QWidget* p = nullptr)
    : Superclass(p)
  {
  }
};

#endif // !pqPluginTreeWidget_h
