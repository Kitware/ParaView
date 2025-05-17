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
#include <QElapsedTimer>
#include <QPair>
#include <QPointer>
#include <QVector>

#include <algorithm>

namespace
{
typedef QPair<double, double> RangeType;
}

//-----------------------------------------------------------------------------
class pqLiveSourceManager::pqInternals
{
public:
  double SpeedMultiplier = 1.;
  double CurrentOffset = 0.;
  bool IsPaused = true;
  QElapsedTimer Timer;
  QVector<QPointer<pqLiveSourceItem>> LiveSources;
  QVector<RangeType> TimeRanges;

  //-----------------------------------------------------------------------------
  /**
   * Iterate over real time reader live sources and create a sorted vector of time ranges
   * that list all non-consecutive ranges.
   */
  void resetTimeRanges()
  {
    this->TimeRanges.clear();
    for (auto& source : this->LiveSources)
    {
      double* timestampRange = source->getTimestampRange();
      if (!source->isEmulatedTimeAlgorithm() || source->isPaused() || !timestampRange)
      {
        continue;
      }

      RangeType currentRange(timestampRange[0], timestampRange[1]);
      if (this->TimeRanges.empty())
      {
        this->TimeRanges.append(currentRange);
        continue;
      }

      bool alreadyExisting = false;
      QVector<RangeType> modifiedRanges;
      for (auto& range : this->TimeRanges)
      {
        if ((currentRange.first < range.first && currentRange.second > range.first) ||
          (currentRange.first < range.second && currentRange.second > range.second))
        {
          modifiedRanges.append(range);
        }
        else if (currentRange.first >= range.first && currentRange.second <= range.second)
        {
          alreadyExisting = true;
        }
      }

      if (!alreadyExisting)
      {
        for (auto& modified : modifiedRanges)
        {
          currentRange.first = std::min(modified.first, currentRange.first);
          currentRange.second = std::max(modified.second, currentRange.second);
          this->TimeRanges.removeOne(modified);
        }
        this->TimeRanges.append(currentRange);
      }
    }

    auto sortRanges = [](const RangeType& firstRange, const RangeType& secondRange)
    { return firstRange.first < secondRange.first; };
    std::sort(this->TimeRanges.begin(), this->TimeRanges.end(), sortRanges);
  }

  //-----------------------------------------------------------------------------
  double getElapsedTime()
  {
    return (static_cast<double>(this->Timer.elapsed()) / 1000.) * this->SpeedMultiplier +
      this->CurrentOffset;
  }

  //-----------------------------------------------------------------------------
  double getCurrentTime()
  {
    if (this->IsPaused)
    {
      return this->CurrentOffset;
    }

    if (this->TimeRanges.empty())
    {
      return 0.;
    }
    if (!this->Timer.isValid())
    {
      return this->TimeRanges.first().first;
    }

    double elapsed = this->getElapsedTime();

    auto findClosestRange = [&elapsed](const RangeType& range) { return elapsed < range.second; };
    auto currentRange =
      std::find_if(this->TimeRanges.begin(), this->TimeRanges.end(), findClosestRange);

    if (currentRange != this->TimeRanges.end() && elapsed < currentRange->first)
    {
      this->CurrentOffset = currentRange->first;
      this->Timer.restart();
      elapsed = currentRange->first;
    }
    return elapsed;
  }
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
  this->pauseEmulatedTime();
}

//-----------------------------------------------------------------------------
void pqLiveSourceManager::resume()
{
  auto& internals = *(this->Internals);

  for (auto& source : internals.LiveSources)
  {
    source->resume();
  }
  this->resumeEmulatedTime();
}

//-----------------------------------------------------------------------------
bool pqLiveSourceManager::isPaused()
{
  auto& sourceList = this->Internals->LiveSources;
  bool paused = std::all_of(sourceList.cbegin(), sourceList.cend(),
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
void pqLiveSourceManager::pauseEmulatedTime()
{
  auto& internals = *(this->Internals);

  if (!internals.IsPaused)
  {
    internals.CurrentOffset = internals.getElapsedTime();
    internals.IsPaused = true;
    Q_EMIT emulatedTimeStateChanged(true);
  }
}

//-----------------------------------------------------------------------------
void pqLiveSourceManager::resumeEmulatedTime()
{
  auto& internals = *(this->Internals);
  if (internals.IsPaused)
  {
    internals.Timer.restart();
    internals.IsPaused = false;
    Q_EMIT emulatedTimeStateChanged(false);
  }
}

//-----------------------------------------------------------------------------
bool pqLiveSourceManager::isEmulatedTimePaused()
{
  return this->Internals->IsPaused;
}

//-----------------------------------------------------------------------------
void pqLiveSourceManager::setEmulatedSpeedMultiplier(double speed)
{
  if (speed < 0 || speed > 100)
  {
    qWarning("Speed multiplier should fall within the range of 0 to 100.");
    return;
  }
  this->Internals->SpeedMultiplier = speed;
}

//-----------------------------------------------------------------------------
double pqLiveSourceManager::getEmulatedSpeedMultiplier()
{
  return this->Internals->SpeedMultiplier;
}

//-----------------------------------------------------------------------------
void pqLiveSourceManager::setEmulatedCurrentTime(double time)
{
  this->Internals->CurrentOffset = time;
  this->Internals->Timer.restart();
}

//-----------------------------------------------------------------------------
double pqLiveSourceManager::getEmulatedCurrentTime()
{
  return this->Internals->getCurrentTime();
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

      this->connect(item, &pqLiveSourceItem::refreshSource,
        [item, &internals]()
        {
          double time = internals.getCurrentTime();
          item->update(time);
        });

      if (item->isEmulatedTimeAlgorithm())
      {
        this->connect(item, &pqLiveSourceItem::onInformationUpdated, this,
          &pqLiveSourceManager::onUpdateTimeRanges);
        this->connect(
          item, &pqLiveSourceItem::stateChanged, this, &pqLiveSourceManager::onUpdateTimeRanges);
      }

      this->Internals->LiveSources.append(item);
      this->Internals->resetTimeRanges();
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

//-----------------------------------------------------------------------------
void pqLiveSourceManager::onUpdateTimeRanges()
{
  this->Internals->resetTimeRanges();
}
