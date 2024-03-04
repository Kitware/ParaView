// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLiveSourceManager.h"

#include "pqApplicationCore.h"
#include "pqLiveSourceItem.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkPVXMLElement.h"
#include "vtkSMViewProxy.h"

#include <QDebug>
#include <QPointer>
#include <QVector>

#include <algorithm>

//-----------------------------------------------------------------------------
class pqLiveSourceManager::pqInternals
{
public:
  QVector<QPointer<pqLiveSourceItem>> LiveSources;
};

//-----------------------------------------------------------------------------
pqLiveSourceManager::pqLiveSourceManager(QObject* parentObject)
  : Superclass(parentObject)
  , Internals(new pqLiveSourceManager::pqInternals())
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(
    smmodel, &pqServerManagerModel::sourceAdded, this, &pqLiveSourceManager::onSourceAdded);
  this->connect(
    smmodel, &pqServerManagerModel::sourceRemoved, this, &pqLiveSourceManager::onSourceRemove);

  Q_FOREACH (pqPipelineSource* src, smmodel->findItems<pqPipelineSource*>())
  {
    this->onSourceAdded(src);
  }
}

//-----------------------------------------------------------------------------
pqLiveSourceManager::~pqLiveSourceManager() = default;

//-----------------------------------------------------------------------------
void pqLiveSourceManager::pause()
{
  auto& internals = *(this->Internals);

  for (auto& source : internals.LiveSources)
  {
    source->pause();
  }
}

//-----------------------------------------------------------------------------
void pqLiveSourceManager::resume()
{
  auto& internals = *(this->Internals);

  for (auto& source : internals.LiveSources)
  {
    source->resume();
  }
}

//-----------------------------------------------------------------------------
bool pqLiveSourceManager::isPaused()
{
  auto& sourceList = this->Internals->LiveSources;
  bool paused = std::any_of(sourceList.cbegin(), sourceList.cend(),
    [](pqLiveSourceItem* item) { return item->isPaused(); });
  return paused;
}

//-----------------------------------------------------------------------------
pqLiveSourceItem* pqLiveSourceManager::getLiveSourceItem(vtkSMProxy* proxy)
{
  if (!proxy)
  {
    return nullptr;
  }

  auto& internals = *(this->Internals);
  auto found = std::find_if(internals.LiveSources.begin(), internals.LiveSources.end(),
    [&proxy](pqLiveSourceItem* item) { return proxy->GetGlobalID() == item->getSourceId(); });
  if (found == internals.LiveSources.end())
  {
    return nullptr;
  }
  return *found;
}

//-----------------------------------------------------------------------------
void pqLiveSourceManager::onSourceAdded(pqPipelineSource* src)
{
  vtkSMProxy* proxy = src->getProxy();
  if (auto hints = proxy->GetHints())
  {
    if (auto liveHints = hints->FindNestedElementByName("LiveSource"))
    {
      pqLiveSourceItem* item = new pqLiveSourceItem(src, liveHints, this);
      auto& internals = *(this->Internals);

      this->connect(
        item, &pqLiveSourceItem::refreshSource, [item, &internals]() { item->update(); });

      this->Internals->LiveSources.append(item);
    }
  }
}

//-----------------------------------------------------------------------------
void pqLiveSourceManager::onSourceRemove(pqPipelineSource* src)
{
  if (!src->getProxy())
  {
    return;
  }

  auto& internals = *(this->Internals);
  vtkTypeUInt32 id = src->getProxy()->GetGlobalID();
  auto found = std::find_if(internals.LiveSources.begin(), internals.LiveSources.end(),
    [&id](pqLiveSourceItem* item) { return id == item->getSourceId(); });
  if (found != internals.LiveSources.end())
  {
    (*found)->disconnect();
    internals.LiveSources.erase(found);
  }
}
