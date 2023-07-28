// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqRecentlyUsedResourcesList.h"

#include "pqSettings.h"

#include <QStringList>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqRecentlyUsedResourcesList::pqRecentlyUsedResourcesList(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqRecentlyUsedResourcesList::~pqRecentlyUsedResourcesList() = default;

//-----------------------------------------------------------------------------
void pqRecentlyUsedResourcesList::add(const pqServerResource& resource)
{
  // Remove any existing resources that match the resource we're about to add ...
  // Note: we consider a resource a "match" if it has the same serverName if any, host(s) and path;
  // we ignore scheme and port(s)
  bool emptyName = resource.serverName().isEmpty();

  for (int cc = 0; cc < this->ResourceList.size(); cc++)
  {
    if ((emptyName && this->ResourceList[cc].hostPath() == resource.hostPath()) ||
      (!emptyName && this->ResourceList[cc].pathServerName() == resource.pathServerName()))
    {
      this->ResourceList.removeAt(cc);
      cc--;
    }
  }

  this->ResourceList.prepend(resource);

  const int max_length = 30;
  while (this->ResourceList.size() > max_length)
  {
    this->ResourceList.removeAt(max_length);
  }

  Q_EMIT this->changed();
}

//-----------------------------------------------------------------------------
void pqRecentlyUsedResourcesList::load(pqSettings& settings)
{
  const QStringList resources = settings.value("RecentlyUsedResourcesList").toStringList();
  this->ResourceList.clear();
  for (int i = resources.size() - 1; i >= 0; --i)
  {
    this->add(pqServerResource(resources[i]));
  }
}

//-----------------------------------------------------------------------------
void pqRecentlyUsedResourcesList::save(pqSettings& settings) const
{
  QStringList resources;
  QList<pqServerResource>::const_iterator iter;
  for (iter = this->ResourceList.begin(); iter != this->ResourceList.end(); ++iter)
  {
    resources.push_back(iter->serializeString());
  }
  settings.setValue("RecentlyUsedResourcesList", resources);
}
