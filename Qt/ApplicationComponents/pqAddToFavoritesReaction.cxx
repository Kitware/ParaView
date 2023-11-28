// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAddToFavoritesReaction.h"

#include "pqActiveObjects.h"
#include "pqPipelineFilter.h"
#include "pqProxyCategory.h"
#include "pqProxyGroupMenuManager.h"

#include "vtkSMProxy.h"

#include <QDebug>
#include <QPointer>

struct pqAddToFavoritesReaction::pqInternal
{
  pqInternal(pqProxyGroupMenuManager* manager)
    : Manager(manager)
  {
  }

  pqProxyGroupMenuManager* Manager = nullptr;
};

//-----------------------------------------------------------------------------
pqAddToFavoritesReaction::pqAddToFavoritesReaction(
  QAction* parentObject, pqProxyGroupMenuManager* manager)
  : Superclass(parentObject)
  , Internal(new pqInternal(manager))
{
  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::sourceChanged, this,
    &pqAddToFavoritesReaction::updateEnableState);

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqAddToFavoritesReaction::pqAddToFavoritesReaction(QAction* parentObject, QVector<QString>& filters)
  : Superclass(parentObject)
  , Internal(new pqInternal(new pqProxyGroupMenuManager(nullptr, QString("ParaViewFilters"))))
{
  Q_UNUSED(filters);
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));

  this->parentAction()->setEnabled(false);
}

//-----------------------------------------------------------------------------
pqAddToFavoritesReaction::~pqAddToFavoritesReaction() = default;

//-----------------------------------------------------------------------------
void pqAddToFavoritesReaction::updateEnableState()
{
  pqPipelineFilter* filter =
    qobject_cast<pqPipelineFilter*>(pqActiveObjects::instance().activeSource());
  if (filter == nullptr || filter->modifiedState() == pqProxy::UNINITIALIZED)
  {
    this->parentAction()->setEnabled(false);
    return;
  }

  auto favorites = this->Internal->Manager->getFavoritesCategory();
  const QString& name = filter->getProxy()->GetXMLName();
  bool exists = favorites->hasProxy(name);

  this->parentAction()->setEnabled(!exists);
}

//-----------------------------------------------------------------------------
void pqAddToFavoritesReaction::addActiveSourceToFavorites()
{
  pqPipelineFilter* filter =
    qobject_cast<pqPipelineFilter*>(pqActiveObjects::instance().activeSource());

  if (!filter)
  {
    qCritical() << "No active filter.";
    return;
  }

  vtkSMProxy* proxy = filter->getProxy();
  const QString name = proxy->GetXMLName();

  auto appCategory = this->Internal->Manager->getApplicationCategory();
  auto proxyInfo = appCategory->findProxy(name, true);
  if (proxyInfo)
  {
    auto favorites = this->Internal->Manager->getFavoritesCategory();
    favorites->addProxy(new pqProxyInfo(favorites, proxyInfo));
    this->Internal->Manager->writeCategoryToSettings();
  }
}
