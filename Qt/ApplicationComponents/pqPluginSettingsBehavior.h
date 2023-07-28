// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPluginSettingsBehavior_h
#define pqPluginSettingsBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

/**
 * @ingroup Behaviors
 * pqPluginSettingsBehavior adds support for adding application settings from plugins
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPluginSettingsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPluginSettingsBehavior(QObject* parent = nullptr);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void updateSettings();
};

#endif
