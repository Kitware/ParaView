// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCustomShortcutBehavior_h
#define pqCustomShortcutBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class QMainWindow;

/**
 * @ingroup Behaviors
 * pqCustomShortcutBehavior TODO
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCustomShortcutBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqCustomShortcutBehavior(QMainWindow* parent = nullptr);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void loadMenuItemShortcuts();

private:
  Q_DISABLE_COPY(pqCustomShortcutBehavior)
};

#endif
