// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqTimelineWidget.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqAnimationTrackEditor.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqPVApplicationCore.h"
#include "pqPropertyLinks.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerModelItem.h"
#include "pqServerManagerObserver.h"
#include "pqTimelineItemDelegate.h"
#include "pqTimelineModel.h"
#include "pqTimelineView.h"
#include "pqUndoStack.h"

#include "vtkCompositeAnimationPlayer.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"

#include <QHeaderView>
#include <QList>
#include <QToolButton>
#include <QVBoxLayout>

using TreeRow = QList<QStandardItem*>;

namespace
{
QString getRegistrationName(vtkSMProxy* proxy)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqProxy* pqproxy = smmodel->findItem<pqProxy*>(proxy);

  return pqproxy ? pqproxy->getSMName() : QString();
}
}

struct pqTimelineWidget::pqInternals
{
  pqTimelineView* TimelineView = nullptr;
  pqTimelineModel* TimeModel = nullptr;
  pqTimelineWidget* Self = nullptr;
  TreeRow TimeKeeperRow;

  vtkNew<vtkEventQtSlotConnect> TimekeeperConnector;

  // Used to avoid updates loop when settings properties from UI modifications.
  bool IsUpdatingSceneFromUIFlag = false;

  // Store each animation cue by registration name.
  QMap<QString, pqAnimationCue*> Cues;

  pqInternals(pqTimelineWidget* self)
    : Self(self)
  {
    this->TimelineView = new pqTimelineView(self);
    this->TimeModel = new pqTimelineModel(self);
    this->TimeModel->setColumnCount(pqTimelineColumn::COUNT);
    this->TimelineView->setModel(this->TimeModel);

    QObject::connect(this->TimeModel, &QAbstractItemModel::rowsInserted,
      [&](const QModelIndex& parent, int first, int) {
        QModelIndexList newIndexes;
        newIndexes << this->TimeModel->index(first, pqTimelineColumn::NAME, parent);
        newIndexes << this->TimeModel->index(first, pqTimelineColumn::TIMELINE, parent);
        newIndexes << this->TimeModel->index(first, pqTimelineColumn::WIDGET, parent);
        this->TimelineView->createRowWidgets(newIndexes);
      });

    QObject::connect(this->TimelineView, &pqTimelineView::validateTrackRequested,
      [&](const pqAnimatedPropertyInfo& propInfo) { this->validateAnimationCue(propInfo); });

    QObject::connect(this->TimelineView, &pqTimelineView::newTrackRequested,
      [&](const pqAnimatedPropertyInfo& propInfo) { this->createAnimationCue(propInfo); });

    QObject::connect(this->TimelineView, &pqTimelineView::deleteTrackRequested,
      [&](const QString& registrationName) { this->deleteAnimationCue(registrationName); });

    QObject::connect(this->TimelineView, &pqTimelineView::editTrackRequested,
      [&](const QString& registrationName) { this->editAnimationCue(registrationName); });

    QObject::connect(this->TimelineView, &pqTimelineView::resetStartEndTimeRequested,
      [&]() { this->resetStartEndTime(); });

    this->TimelineView->setHeaderHidden(true);
    this->TimelineView->setIndentation(10);

    auto delegate = new pqTimelineItemDelegate(self, this->TimelineView);
    this->TimelineView->setItemDelegateForColumn(pqTimelineColumn::TIMELINE, delegate);
    // allow to handle mouse events in delegate
    this->TimelineView->viewport()->setMouseTracking(true);
    this->TimelineView->viewport()->installEventFilter(delegate);

    QObject::connect(delegate, &pqTimelineItemDelegate::needsRepaint,
      [&]() { this->TimelineView->updateTimelines(); });

    // create time header row
    QMap<int, QVariant> timeData;
    timeData[Qt::ToolTipRole] = tr("If checked, scene use times from time sources.\nOtherwise, "
                                   "generate NumberOfFrames time entries.");
    this->TimeModel->createRow(pqTimelineTrack::TIME, tr("Time Sources"), timeData);
    this->TimeModel->setRowEnabled(pqTimelineTrack::TIME, false);

    // create animation header row
    QMap<int, QVariant> animationData;
    animationData[Qt::ToolTipRole] = tr("Check / Uncheck to enable all animation tracks.");
    this->TimeModel->createRow(pqTimelineTrack::ANIMATION_HEADER, "Animations", animationData);

    auto header = this->TimelineView->header();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(pqTimelineColumn::TIMELINE, QHeaderView::Stretch);
    header->setSectionResizeMode(pqTimelineColumn::WIDGET, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(pqTimelineColumn::NAME, QHeaderView::ResizeToContents);

    auto vlay = new QVBoxLayout(self);
    vlay->addWidget(this->TimelineView);
  }

  // store list of times under TIMES data role
  void setTrackTimes(const TreeRow& row, vtkSMProxy* proxy)
  {
    QVariantList times;
    vtkSMDoubleVectorProperty* dvp =
      vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty("TimestepValues"));
    if (dvp)
    {
      unsigned int numElems = dvp->GetNumberOfElements();
      for (unsigned int cc = 0; cc < numElems; cc++)
      {
        times.append(dvp->GetElement(cc));
      }
    }

    for (auto item : row)
    {
      item->setData(times, pqTimelineItemRole::TIMES);
    }
  }

  // delete cue from the scene
  void deleteAnimationCue(const QString& registrationName)
  {
    auto cue = this->Cues[registrationName];
    if (cue)
    {
      pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
      pqAnimationScene* scene = mgr->getActiveScene();
      BEGIN_UNDO_SET(tr("Remove Animation Track"));
      SM_SCOPED_TRACE(Delete).arg(cue->getProxy());
      // The timeline model will be automatically updated by the cue removal.
      scene->removeCue(cue);
      END_UNDO_SET();
    }
  }

  // Return true if track can be added:
  // - always true for python track
  // - only one camera per view (so only proxy matters)
  // - only one (Proxy, Property) pair in other cases
  bool canTrackBeAdded(const pqAnimatedPropertyInfo& propInfo)
  {
    if (propInfo.Proxy == nullptr)
    {
      return true;
    }

    // look in existing cues
    for (auto cue : this->Cues)
    {
      // skip python
      if (!cue || cue->isPythonCue())
      {
        continue;
      }

      // skip other proxies
      if (cue->getAnimatedProxy() != propInfo.Proxy)
      {
        continue;
      }

      if (cue->isCameraCue() ||
        (cue->getAnimatedPropertyName() == propInfo.Name &&
          cue->getAnimatedPropertyIndex() == propInfo.Index))
      {
        return false;
      }
    }

    return true;
  }

  void validateAnimationCue(const pqAnimatedPropertyInfo& propInfo)
  {
    this->TimelineView->enableTrackCreationWidget(this->canTrackBeAdded(propInfo));
  }

  // add a cue in the scene, and update internal cache of cue list.
  // sanity check to avoid duplicates.
  void createAnimationCue(const pqAnimatedPropertyInfo& propInfo)
  {
    pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
    pqAnimationScene* scene = mgr->getActiveScene();

    // no proxy means python track.
#if !VTK_MODULE_ENABLE_ParaView_pqPython
    if (!propInfo.Proxy)
    {
      qCritical() << "Python support not enabled. Please recompile ParaView "
                     "with Python enabled.";
      return;
    }
#endif

    if (!this->canTrackBeAdded(propInfo))
    {
      qWarning("Animation track already exists.");
      return;
    }

    BEGIN_UNDO_SET(tr("Add Animation Track"));
    // The timeline model will be automatically updated by the cue creation.
    auto cue = scene->createCue(propInfo);
    SM_SCOPED_TRACE(CreateAnimationTrack).arg("cue", cue->getProxy());
    this->Cues[cue->getSMName()] = cue;
    END_UNDO_SET();

    this->TimelineView->enableTrackCreationWidget(this->canTrackBeAdded(propInfo));
  }

  // show cue editor
  void editAnimationCue(const QString& registrationName)
  {
    if (!this->Cues.contains(registrationName))
    {
      return;
    }

    auto cue = this->Cues[registrationName];
    if (cue)
    {
      pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
      pqAnimationScene* scene = mgr->getActiveScene();
      auto editor = new pqAnimationTrackEditor(scene, cue);
      editor->showEditor();
    }
  }

  // update lists stored under TIMES and LABELS data role for given cue, based on keyframes info.
  void updateCueKeyframesData(pqAnimationCue* cue, const TreeRow& row)
  {
    QList<vtkSMProxy*> newKeyFrames = cue->getKeyFrames();
    QVariantList newValues;
    QVariantList newTimes;
    for (auto frame : newKeyFrames)
    {
      newTimes << vtkSMPropertyHelper(frame->GetProperty("KeyTime")).GetAsDouble();
      if (!cue->isPythonCue() && !cue->isCameraCue())
      {
        newValues << vtkSMPropertyHelper(frame->GetProperty("KeyValues")).GetAsDouble();
      }
    }

    for (auto item : row)
    {
      item->setData(newTimes, pqTimelineItemRole::TIMES);
      item->setData(newValues, pqTimelineItemRole::LABELS);
    }
  }

  // set scene start and end time based on timekeeper.
  void resetStartEndTime()
  {
    pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
    auto scene = mgr->getActiveScene();
    if (!scene)
    {
      return;
    }

    auto times = scene->getTimeSteps();

    if (!times.empty())
    {
      auto sceneProxy = scene->getProxy();
      vtkSMPropertyHelper(sceneProxy->GetProperty("StartTime")).Set(times.first());
      vtkSMPropertyHelper(sceneProxy->GetProperty("EndTime")).Set(times.last());
      sceneProxy->UpdateVTKObjects();
    }
  }

  // add a SOURCE row in the model
  TreeRow createSourceRow(vtkSMProxy* proxy)
  {
    QMap<int, QVariant> additionalData;
    additionalData[pqTimelineItemRole::PROXY] = QVariant::fromValue(static_cast<void*>(proxy));
    additionalData[Qt::ToolTipRole] =
      tr("Check/Uncheck to make timesteps available in the scene time list.");

    auto row = this->TimeModel->createRow(
      pqTimelineTrack::SOURCE, ::getRegistrationName(proxy), additionalData);
    this->setTrackTimes(row, proxy);

    return row;
  }

  // return timekeeper for active server.
  vtkSMProxy* activeTimeKeeper()
  {
    auto server = pqActiveObjects::instance().activeServer();
    if (!server)
    {
      return nullptr;
    }

    vtkNew<vtkSMParaViewPipelineController> controller;
    return controller->FindTimeKeeper(server->session());
  }
};

//-----------------------------------------------------------------------------
pqTimelineWidget::pqTimelineWidget(QWidget* parent)
  : Superclass(parent)
  , Internals(new pqTimelineWidget::pqInternals(this))
{
  pqCoreUtilities::connect(vtkSMProxyManager::GetProxyManager(),
    vtkSMProxyManager::ActiveSessionChanged, this, SLOT(onNewSession()));

  pqServerManagerObserver* smobserver = pqApplicationCore::instance()->getServerManagerObserver();

  this->connect(smobserver, SIGNAL(proxyUnRegistered(const QString&, const QString&, vtkSMProxy*)),
    this, SLOT(updateCueCache(const QString&, const QString&, vtkSMProxy*)));

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(
    smmodel, &pqServerManagerModel::nameChanged, this, &pqTimelineWidget::renameTrackFromProxy);
  this->connect(smmodel, &pqServerManagerModel::dataUpdated, this,
    &pqTimelineWidget::updateSourceTimeFromProxy);

  this->connect(smmodel, &pqServerManagerModel::sourceRemoved, this,
    &pqTimelineWidget::updateSceneFromEnabledSources);
  this->connect(
    smmodel, &pqServerManagerModel::aboutToRemoveServer, this, &pqTimelineWidget::onNewSession);

  this->connect(this->Internals->TimeModel, &pqTimelineModel::sourceStateChanged, this,
    &pqTimelineWidget::updateSceneFromEnabledSources);
  this->connect(this->Internals->TimeModel, &pqTimelineModel::animationStateChanged, this,
    &pqTimelineWidget::updateSceneFromEnabledAnimations);

  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  this->connect(mgr, &pqAnimationManager::activeSceneChanged, [&](pqAnimationScene* scene) {
    if (scene)
    {
      this->connect(scene, &pqAnimationScene::playModeChanged, this,
        &pqTimelineWidget::updateTimeTrackFromScene);
      this->connect(scene, &pqAnimationScene::clockTimeRangesChanged, this,
        &pqTimelineWidget::updateTimeTrackFromScene);
      this->connect(scene, &pqAnimationScene::frameCountChanged, this,
        &pqTimelineWidget::updateTimeTrackFromScene);
      this->connect(scene, &pqAnimationScene::addedCue, this, &pqTimelineWidget::addCueTrack);
      this->connect(
        scene, &pqAnimationScene::preRemovedCue, this, &pqTimelineWidget::removeCueTrack);

      // add already existing cues, as TimeKeeper
      for (auto cue : scene->getCues())
      {
        this->addCueTrack(cue);
      }

      this->updateTimeTrackFromScene();
    }
  });
}

//-----------------------------------------------------------------------------
pqTimelineWidget::~pqTimelineWidget() = default;

//-----------------------------------------------------------------------------
void pqTimelineWidget::setAdvancedMode(bool advanced)
{
  if (this->Internals->TimeKeeperRow.isEmpty())
  {
    return;
  }

  QModelIndex keeperFirstIndex =
    this->Internals->TimeModel->indexFromItem(this->Internals->TimeKeeperRow.first());
  this->Internals->TimelineView->setRowHidden(
    keeperFirstIndex.row(), keeperFirstIndex.parent(), !advanced);
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::updateTimeTrackFromScene()
{
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  auto scene = mgr->getActiveScene();
  if (!scene)
  {
    return;
  }

  auto sceneProxy = scene->getProxy();

  // update source time rows
  auto playMode = vtkSMPropertyHelper(sceneProxy->GetProperty("PlayMode")).GetAsInt();
  bool snapToTimestep = playMode == vtkCompositeAnimationPlayer::SNAP_TO_TIMESTEPS;
  auto nbOfFrames = vtkSMPropertyHelper(sceneProxy->GetProperty("NumberOfFrames")).GetAsInt();

  auto timeRow = this->Internals->TimeModel->rows(pqTimelineTrack::TIME).first();
  if (!snapToTimestep && nbOfFrames > 1)
  {
    auto start = scene->getClockTimeRange().first;
    auto end = scene->getClockTimeRange().second;
    QVariantList times;
    for (int i = 0; i < nbOfFrames; i++)
    {
      times << start + i * (end - start) / (nbOfFrames - 1);
    }

    timeRow[pqTimelineColumn::TIMELINE]->setData(times, pqTimelineItemRole::TIMES);
  }
  else if (snapToTimestep)
  {
    this->Internals->setTrackTimes(timeRow, this->Internals->activeTimeKeeper());
  }

  if (!this->Internals->IsUpdatingSceneFromUIFlag)
  {
    this->Internals->TimeModel->setRowEnabled(pqTimelineTrack::TIME, snapToTimestep);
  }
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::onNewSession()
{
  this->Internals->TimekeeperConnector->Disconnect();

  this->Internals->Cues.clear();
  this->Internals->TimeModel->clearRows(pqTimelineTrack::ANIMATION);

  vtkSMProxy* timeKeeper = this->Internals->activeTimeKeeper();
  if (timeKeeper)
  {
    this->Internals->TimekeeperConnector->Connect(timeKeeper->GetProperty("TimeSources"),
      vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(recreateTimeSources()));
    this->Internals->TimekeeperConnector->Connect(timeKeeper->GetProperty("TimestepValues"),
      vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(recreateTimeSources()));
    this->Internals->TimekeeperConnector->Connect(timeKeeper->GetProperty("SuppressedTimeSources"),
      vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(recreateTimeSources()));
    this->recreateTimeSources();
  }
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::recreateTimeSources()
{
  vtkSMProxy* timeKeeper = this->Internals->activeTimeKeeper();

  auto timeRow = this->Internals->TimeModel->rows(pqTimelineTrack::TIME).first();
  for (auto item : timeRow)
  {
    item->setData(QVariant::fromValue(static_cast<void*>(timeKeeper)), pqTimelineItemRole::PROXY);
  }
  this->updateTimeTrackFromScene();

  // if we come here because of a UI modification, do not re-update the ui.
  if (this->Internals->IsUpdatingSceneFromUIFlag)
  {
    return;
  }

  this->Internals->TimeModel->clearRows(pqTimelineTrack::SOURCE);
  auto sourcesProp = vtkSMProxyProperty::SafeDownCast(timeKeeper->GetProperty("TimeSources"));
  int nbOfTimeSources = sourcesProp->GetNumberOfProxies();
  for (int idx = 0; idx < nbOfTimeSources; idx++)
  {
    auto sourceProxy = sourcesProp->GetProxy(idx);
    if (sourceProxy &&
      (sourceProxy->GetProperty("TimestepValues") || sourceProxy->GetProperty("TimeRange")))
    {
      auto row = this->Internals->createSourceRow(sourceProxy);
    }
  }

  auto uncheckedSources =
    vtkSMProxyProperty::SafeDownCast(timeKeeper->GetProperty("SuppressedTimeSources"));
  nbOfTimeSources = uncheckedSources->GetNumberOfProxies();
  for (int idx = 0; idx < nbOfTimeSources; idx++)
  {
    auto sourceProxy = uncheckedSources->GetProxy(idx);
    if (sourceProxy)
    {
      this->Internals->TimeModel->setRowEnabled(
        pqTimelineTrack::SOURCE, false, ::getRegistrationName(sourceProxy));
    }
  }
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::updateSceneFromEnabledSources()
{
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* scene = mgr->getActiveScene();
  if (!scene)
  {
    return;
  }

  auto timeRange = scene->getClockTimeRange();

  this->Internals->IsUpdatingSceneFromUIFlag = true;
  // update timekeeper
  vtkSMProxy* timeKeeper = this->Internals->activeTimeKeeper();
  auto uncheckedSourcesProp =
    vtkSMProxyProperty::SafeDownCast(timeKeeper->GetProperty("SuppressedTimeSources"));

  auto rows = this->Internals->TimeModel->uncheckedRows(pqTimelineTrack::SOURCE);

  uncheckedSourcesProp->RemoveAllProxies();
  std::vector<vtkSMProxy*> proxies;
  for (const auto& row : rows)
  {
    QVariant proxyVariant = row[pqTimelineColumn::NAME]->data(pqTimelineItemRole::PROXY);
    vtkSMProxy* proxy = reinterpret_cast<vtkSMProxy*>(proxyVariant.value<void*>());
    proxies.push_back(proxy);
  }
  uncheckedSourcesProp->SetProxies(static_cast<unsigned int>(proxies.size()), proxies.data());
  timeKeeper->UpdateVTKObjects();

  // update scene
  vtkSMProxy* sceneProxy = scene->getProxy();

  vtkSMPropertyHelper playModeHelper(sceneProxy->GetProperty("PlayMode"));
  int prevMode = playModeHelper.GetAsInt();
  bool snapToTimeStep = this->Internals->TimeModel->isRowEnabled(pqTimelineTrack::TIME);
  int newMode = snapToTimeStep ? vtkCompositeAnimationPlayer::SNAP_TO_TIMESTEPS
                               : vtkCompositeAnimationPlayer::SEQUENCE;

  // keep end and start times when mode changes to Sequence
  if (!snapToTimeStep && prevMode != newMode)
  {
    vtkSMPropertyHelper(sceneProxy->GetProperty("StartTime")).Set(timeRange.first);
    vtkSMPropertyHelper(sceneProxy->GetProperty("EndTime")).Set(timeRange.second);
  }

  playModeHelper.Set(newMode);

  sceneProxy->UpdateVTKObjects();

  this->Internals->IsUpdatingSceneFromUIFlag = false;
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::updateSceneFromEnabledAnimations()
{
  auto tracks = this->Internals->TimeModel->rows(pqTimelineTrack::ANIMATION);
  for (const auto& track : tracks)
  {
    QString name =
      track[pqTimelineColumn::NAME]->data(pqTimelineItemRole::REGISTRATION_NAME).toString();
    bool enable = this->Internals->TimeModel->isRowEnabled(pqTimelineTrack::ANIMATION, name);
    if (this->Internals->Cues.contains(name))
    {
      this->Internals->Cues[name]->setEnabled(enable);
    }
  }
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::renameTrackFromProxy(pqServerManagerModelItem* item)
{
  auto pqproxy = dynamic_cast<pqProxy*>(item);
  if (pqproxy)
  {
    for (auto row : this->Internals->TimeModel->rows(pqTimelineTrack::SOURCE))
    {
      auto proxyVariant = row[pqTimelineColumn::NAME]->data(pqTimelineItemRole::PROXY);
      vtkSMProxy* proxy = reinterpret_cast<vtkSMProxy*>(proxyVariant.value<void*>());
      if (proxy == pqproxy->getProxy())
      {
        row[pqTimelineColumn::NAME]->setData(pqproxy->getSMName(), Qt::DisplayRole);
        for (auto rowItem : row)
        {
          rowItem->setData(pqproxy->getSMName(), pqTimelineItemRole::REGISTRATION_NAME);
        }
        return;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::updateSourceTimeFromProxy(pqServerManagerModelItem* item)
{
  pqProxy* pqproxy = qobject_cast<pqProxy*>(item);
  if (pqproxy)
  {
    for (auto row : this->Internals->TimeModel->rows(pqTimelineTrack::SOURCE))
    {
      auto proxyVariant = row[pqTimelineColumn::NAME]->data(pqTimelineItemRole::PROXY);
      vtkSMProxy* proxy = reinterpret_cast<vtkSMProxy*>(proxyVariant.value<void*>());
      if (proxy == pqproxy->getProxy())
      {
        vtkPVDataInformation* dinfo = vtkSMSourceProxy::SafeDownCast(proxy)->GetDataInformation(0);
        row[pqTimelineColumn::TIMELINE]->setData(dinfo->GetTime(), pqTimelineItemRole::SOURCE_TIME);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::addCueTrack(pqAnimationCue* cue)
{
  QMap<int, QVariant> additionalData;
  additionalData[pqTimelineItemRole::PROXY] =
    QVariant::fromValue(static_cast<void*>(cue->getProxy()));
  additionalData[pqTimelineItemRole::REGISTRATION_NAME] = cue->getSMName();
  additionalData[Qt::ToolTipRole] =
    tr("Check / uncheck to enable the animation. Double click on name or timeline to edit.");

  auto newRow = this->Internals->TimeModel->createRow(
    pqTimelineTrack::ANIMATION, cue->getDisplayName(), additionalData);
  if (cue->isTimekeeperCue())
  {
    this->Internals->TimeKeeperRow = newRow;
    QString toolTip = tr("Timekeeper updates pipeline. It maps scene time to a requested pipeline "
                         "time.\n Uncheck to freeze pipeline time.");
    newRow[pqTimelineColumn::NAME]->setData(toolTip, Qt::ToolTipRole);
    newRow[pqTimelineColumn::TIMELINE]->setData(toolTip, Qt::ToolTipRole);
  }

  this->Internals->Cues[cue->getSMName()] = cue;
  this->Internals->updateCueKeyframesData(cue, newRow);

  this->connect(
    cue, &pqAnimationCue::keyframesModified, this, &pqTimelineWidget::updateCueTracksData);
  this->connect(cue, &pqAnimationCue::enabled, this, &pqTimelineWidget::updateCueTracksState);
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::removeCueTrack(pqAnimationCue* cue)
{
  this->Internals->Cues.remove(cue->getSMName());
  this->Internals->TimeModel->deleteRow(pqTimelineTrack::ANIMATION, cue->getSMName());

  this->Internals->TimelineView->validateTrackCreationWidget();
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::updateCueTracksState()
{
  auto animationRows = this->Internals->TimeModel->rows(pqTimelineTrack::ANIMATION);
  for (auto row : animationRows)
  {
    QString name =
      row[pqTimelineColumn::NAME]->data(pqTimelineItemRole::REGISTRATION_NAME).toString();
    auto cue = this->Internals->Cues[name];
    if (!cue)
    {
      continue;
    }

    bool enabled = cue->isEnabled();
    this->Internals->TimeModel->setRowEnabled(pqTimelineTrack::ANIMATION, enabled, name);
  }
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::updateCueTracksData()
{
  auto animationRows = this->Internals->TimeModel->rows(pqTimelineTrack::ANIMATION);
  for (auto row : animationRows)
  {
    QString name =
      row[pqTimelineColumn::NAME]->data(pqTimelineItemRole::REGISTRATION_NAME).toString();
    auto cue = this->Internals->Cues[name];
    if (!cue)
    {
      continue;
    }

    this->Internals->updateCueKeyframesData(cue, row);
  }
}

//-----------------------------------------------------------------------------
void pqTimelineWidget::updateCueCache(const QString& group, const QString& name, vtkSMProxy* proxy)
{
  Q_UNUSED(group);
  Q_UNUSED(proxy);
  this->Internals->Cues.remove(name);
}
