// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqVRAvatarEvents.h"
#include "ui_pqVRAvatarEvents.h"

#include "pqVRConnectionManager.h"
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
#include "pqVRPNConnection.h"
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
#include "pqVRUIConnection.h"
#endif

#include "vtkStringList.h"

#include <QComboBox>
#include <QDebug>
#include <QStringList>

#include <algorithm>
#include <map>
#include <string>

class pqVRAvatarEvents::pqInternals : public Ui::VRAvatarEvents
{
public:
  void UpdateTrackerNames()
  {
    // Populate input lists
    pqVRConnectionManager* mgr = pqVRConnectionManager::instance();
    this->TrackerNames.clear();
    std::map<std::string, std::string> trackers;

    Q_FOREACH (const QString& connName, mgr->connectionNames())
    {
      trackers.clear();
      // Lookup connection
      bool found = false;
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
      if (pqVRPNConnection* conn = mgr->GetVRPNConnection(connName))
      {
        trackers = conn->trackerMap();
        found = true;
      }
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
      if (pqVRUIConnection* conn = mgr->GetVRUIConnection(connName))
      {
        if (!found)
        {
          trackers = conn->trackerMap();
          found = true;
        }
      }
#endif
      if (!found)
      {
        qDebug() << "Missing connection? Name:" << connName;
        continue;
      }

      std::map<std::string, std::string>::const_iterator it;
      std::map<std::string, std::string>::const_iterator it_end;

      for (it = trackers.begin(), it_end = trackers.end(); it != it_end; ++it)
      {
        this->TrackerNames.append(
          QString("%1.%2").arg(connName).arg(QString::fromStdString(it->second)));
      }
    }

    std::sort(this->TrackerNames.begin(), this->TrackerNames.end());
    this->TrackerNames.insert(0, "");
  }

  bool navigationSharingEnabled;
  QStringList TrackerNames;
  QMap<AvatarEventType, QString> avatarEventMap;
};

//-----------------------------------------------------------------------------
pqVRAvatarEvents::pqVRAvatarEvents(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqInternals())
{
  this->Internals->setupUi(this);

  this->Internals->navigationSharingEnabled = false;

  connect(this->Internals->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(this->Internals->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

//-----------------------------------------------------------------------------
pqVRAvatarEvents::~pqVRAvatarEvents()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqVRAvatarEvents::getEventName(AvatarEventType type, QString& eventName)
{
  eventName = this->Internals->avatarEventMap[type];
}

//-----------------------------------------------------------------------------
void pqVRAvatarEvents::setEventName(AvatarEventType type, QString& eventName)
{
  this->Internals->avatarEventMap[type] = eventName;
}

//-----------------------------------------------------------------------------
bool pqVRAvatarEvents::getNavigationSharing()
{
  return this->Internals->navigationSharingEnabled;
}

//-----------------------------------------------------------------------------
void pqVRAvatarEvents::setNavigationSharing(bool enabled)
{
  this->Internals->navigationSharingEnabled = enabled;
}

//-----------------------------------------------------------------------------
void pqVRAvatarEvents::accept()
{
  this->Internals->avatarEventMap[Head] = this->Internals->avatarHeadEventCombo->currentText();
  this->Internals->avatarEventMap[LeftHand] =
    this->Internals->avatarLeftHandEventCombo->currentText();
  this->Internals->avatarEventMap[RightHand] =
    this->Internals->avatarRightHandEventCombo->currentText();

  this->Internals->navigationSharingEnabled = this->Internals->cShareNavigation->isChecked();

  this->Superclass::accept();
}

//-----------------------------------------------------------------------------
int pqVRAvatarEvents::exec()
{
  this->Internals->UpdateTrackerNames();

  this->Internals->avatarHeadEventCombo->clear();
  this->Internals->avatarHeadEventCombo->addItems(this->Internals->TrackerNames);

  if (this->Internals->TrackerNames.contains(this->Internals->avatarEventMap[Head]))
  {
    this->Internals->avatarHeadEventCombo->setCurrentText(this->Internals->avatarEventMap[Head]);
  }

  this->Internals->avatarLeftHandEventCombo->clear();
  this->Internals->avatarLeftHandEventCombo->addItems(this->Internals->TrackerNames);

  if (this->Internals->TrackerNames.contains(this->Internals->avatarEventMap[LeftHand]))
  {
    this->Internals->avatarLeftHandEventCombo->setCurrentText(
      this->Internals->avatarEventMap[LeftHand]);
  }

  this->Internals->avatarRightHandEventCombo->clear();
  this->Internals->avatarRightHandEventCombo->addItems(this->Internals->TrackerNames);

  if (this->Internals->TrackerNames.contains(this->Internals->avatarEventMap[RightHand]))
  {
    this->Internals->avatarRightHandEventCombo->setCurrentText(
      this->Internals->avatarEventMap[RightHand]);
  }

  // Sync checkbox state
  this->Internals->cShareNavigation->setChecked(this->Internals->navigationSharingEnabled);

  return this->Superclass::exec();
}
