// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAutoLoadPluginXMLBehavior_h
#define pqAutoLoadPluginXMLBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QSet>

/**
 * @ingroup Behaviors
 *
 * ParaView plugins can load gui configuration xmls eg. xmls for defining the
 * filters menu, readers etc. This behavior ensures that as soon as such
 * plugins are loaded if they provide any XMLs in the ":/.{*}/ParaViewResources/"
 * resource location, then such xmls are parsed and an attempt is made to load
 * them (by calling pqApplicationCore::loadConfiguration()).
 *
 * Note: {} is to keep the *+/ from ending the comment block.
 *
 * Without going into too much detail, if you want your application to
 * automatically load GUI configuration XMLs from plugins, instantiate this
 * behavior.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqAutoLoadPluginXMLBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqAutoLoadPluginXMLBehavior(QObject* parent = nullptr);

protected Q_SLOTS:
  void updateResources();

private:
  Q_DISABLE_COPY(pqAutoLoadPluginXMLBehavior)
  QSet<QString> PreviouslyParsedResources;
  QSet<QString> PreviouslyParsedPlugins;
};

#endif
