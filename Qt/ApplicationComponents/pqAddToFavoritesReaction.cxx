// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAddToFavoritesReaction.h"
#include "pqQtDeprecated.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqSettings.h"

#include "vtkSMProxy.h"

#include <QDebug>
//-----------------------------------------------------------------------------
pqAddToFavoritesReaction::pqAddToFavoritesReaction(QAction* parentObject, QVector<QString>& filters)
  : Superclass(parentObject)
{
  this->Filters.swap(filters);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));

  this->updateEnableState();
}

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

  this->parentAction()->setEnabled(!this->Filters.contains(filter->getProxy()->GetXMLName()));
}

//-----------------------------------------------------------------------------
void pqAddToFavoritesReaction::addToFavorites(QAction* parent)
{
  pqPipelineFilter* filter =
    qobject_cast<pqPipelineFilter*>(pqActiveObjects::instance().activeSource());

  if (!filter)
  {
    qCritical() << "No active filter.";
    return;
  }

  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString key = QString("favorites.%1/").arg("ParaViewFilters");

  vtkSMProxy* proxy = filter->getProxy();
  QString filterId = QString("%1;%2;%3;%4")
                       .arg(proxy->GetXMLGroup())
                       .arg(parent->data().toString())
                       .arg(proxy->GetXMLLabel())
                       .arg(proxy->GetXMLName());

  QString value;
  if (settings->contains(key))
  {
    value = settings->value(key).toString();
  }
  QString settingValue = settings->value(key).toString();
  QStringList bmList = settingValue.split("|", PV_QT_SKIP_EMPTY_PARTS);
  for (const QString& bm : bmList)
  {
    if (bm == filterId)
    {
      return;
    }
  }
  value += (filterId + QString("|"));
  settings->setValue(key, value);
}
