// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqVerifyRequiredPluginBehavior_h
#define pqVerifyRequiredPluginBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QSet>

/**
 * @ingroup Behaviors
 *
 * ParaView plugins can be loaded on the client or server before a connection is made.
 * This class makes sure if a plugin that needs both client and server components
 * is not correctly located we can inform the user.
 */

class PQAPPLICATIONCOMPONENTS_EXPORT pqVerifyRequiredPluginBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqVerifyRequiredPluginBehavior(QObject* parent = nullptr);

protected Q_SLOTS:
  void requiredPluginsNotLoaded();

private:
  Q_DISABLE_COPY(pqVerifyRequiredPluginBehavior)
  QSet<QString> PreviouslyParsedResources;
};

#endif
