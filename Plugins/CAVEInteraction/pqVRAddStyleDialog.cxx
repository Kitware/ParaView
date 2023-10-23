// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqVRAddStyleDialog.h"
#include "ui_pqVRAddStyleDialog.h"

#include "pqVRConnectionManager.h"
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
#include "pqVRPNConnection.h"
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
#include "pqVRUIConnection.h"
#endif

#include "vtkNew.h"
#include "vtkSMProxy.h"
#include "vtkSMVRInteractorStyleProxy.h"
#include "vtkStringList.h"

#include <QComboBox>

#include <QtCore/QDebug>
#include <QtCore/QStringList>

#include <algorithm>
#include <map>
#include <string>

class pqVRAddStyleDialog::pqInternals : public Ui::VRAddStyleDialog
{
public:
  bool CanConfigure;
  vtkSMVRInteractorStyleProxy* Style;

  enum InputType
  {
    Valuator = 0,
    Button,
    Tracker
  };

  struct InputGui
  {
    InputType type;
    std::string role;
    QComboBox* combo;
  };

  void AddInput(QWidget* parent, InputType type, const std::string& role, const std::string& name);
  QList<InputGui> Inputs;

  QStringList ValuatorNames;
  QStringList ButtonNames;
  QStringList TrackerNames;
};

//-----------------------------------------------------------------------------
pqVRAddStyleDialog::pqVRAddStyleDialog(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqInternals())
{
  this->Internals->setupUi(this);
  this->Internals->CanConfigure = false;

  // Populate input lists
  pqVRConnectionManager* mgr = pqVRConnectionManager::instance();

  std::map<std::string, std::string> valuators;
  std::map<std::string, std::string> buttons;
  std::map<std::string, std::string> trackers;

  Q_FOREACH (const QString& connName, mgr->connectionNames())
  {
    valuators.clear();
    buttons.clear();
    trackers.clear();
    // Lookup connection
    bool found = false;
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
    if (pqVRPNConnection* conn = mgr->GetVRPNConnection(connName))
    {
      valuators = conn->valuatorMap();
      buttons = conn->buttonMap();
      trackers = conn->trackerMap();
      found = true;
    }
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
    if (pqVRUIConnection* conn = mgr->GetVRUIConnection(connName))
    {
      if (!found)
      {
        valuators = conn->valuatorMap();
        buttons = conn->buttonMap();
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
    for (it = valuators.begin(), it_end = valuators.end(); it != it_end; ++it)
    {
      this->Internals->ValuatorNames.append(
        QString("%1.%2").arg(connName).arg(QString::fromStdString(it->second)));
    }
    for (it = buttons.begin(), it_end = buttons.end(); it != it_end; ++it)
    {
      this->Internals->ButtonNames.append(
        QString("%1.%2").arg(connName).arg(QString::fromStdString(it->second)));
    }
    for (it = trackers.begin(), it_end = trackers.end(); it != it_end; ++it)
    {
      this->Internals->TrackerNames.append(
        QString("%1.%2").arg(connName).arg(QString::fromStdString(it->second)));
    }
  }

  std::sort(this->Internals->ValuatorNames.begin(), this->Internals->ValuatorNames.end());
  std::sort(this->Internals->ButtonNames.begin(), this->Internals->ButtonNames.end());
  std::sort(this->Internals->TrackerNames.begin(), this->Internals->TrackerNames.end());
}

//-----------------------------------------------------------------------------
pqVRAddStyleDialog::~pqVRAddStyleDialog()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqVRAddStyleDialog::setInteractorStyle(vtkSMVRInteractorStyleProxy* style, const QString& name)
{
  this->Internals->Style = style;
  this->Internals->infoLabel->setText(tr("Configuring style %1.").arg(name));

  // Create gui
  vtkNew<vtkStringList> roles;
  style->GetValuatorRoles(roles.GetPointer());
  for (int i = 0; i < roles->GetNumberOfStrings(); ++i)
  {
    std::string role(roles->GetString(i));
    std::string valuatorName(style->GetValuatorName(role));
    this->Internals->AddInput(this, pqInternals::Valuator, role, valuatorName);
  }
  style->GetButtonRoles(roles.GetPointer());
  for (int i = 0; i < roles->GetNumberOfStrings(); ++i)
  {
    std::string role(roles->GetString(i));
    std::string buttonName(style->GetButtonName(role));
    this->Internals->AddInput(this, pqInternals::Button, role, buttonName);
  }
  style->GetTrackerRoles(roles.GetPointer());
  for (int i = 0; i < roles->GetNumberOfStrings(); ++i)
  {
    std::string role(roles->GetString(i));
    std::string trackerName(style->GetTrackerName(role));
    this->Internals->AddInput(this, pqInternals::Tracker, role, trackerName);
  }

  this->Internals->CanConfigure = (this->Internals->Inputs.size() != 0);
}

//-----------------------------------------------------------------------------
void pqVRAddStyleDialog::updateInteractorStyle()
{
  if (!this->Internals->CanConfigure || !this->Internals->Style)
  {
    return;
  }

  Q_FOREACH (const pqInternals::InputGui& gui, this->Internals->Inputs)
  {
    const std::string& role = gui.role;
    const std::string& name = gui.combo->currentText().toStdString();
    switch (gui.type)
    {
      case pqInternals::Valuator:
        this->Internals->Style->SetValuatorName(role, name);
        break;
      case pqInternals::Button:
        this->Internals->Style->SetButtonName(role, name);
        break;
      case pqInternals::Tracker:
        this->Internals->Style->SetTrackerName(role, name);
        break;
      default:
        break;
    }
  }
}

//-----------------------------------------------------------------------------
bool pqVRAddStyleDialog::isConfigurable()
{
  return this->Internals->CanConfigure;
}

//-----------------------------------------------------------------------------
void pqVRAddStyleDialog::pqInternals::AddInput(
  QWidget* parent, InputType type, const std::string& role, const std::string& name)
{
  this->Inputs.push_back(InputGui());
  InputGui& gui = this->Inputs.back();

  gui.type = type;
  gui.role = role;
  gui.combo = new QComboBox(parent);
  switch (type)
  {
    case Valuator:
      gui.combo->addItems(this->ValuatorNames);
      break;
    case Button:
      gui.combo->addItems(this->ButtonNames);
      break;
    case Tracker:
      gui.combo->addItems(this->TrackerNames);
      break;
    default:
      qWarning() << "Unknown tracker type: " << type;
  }

  int comboIndex = gui.combo->findText(QString::fromStdString(name));
  if (comboIndex != -1)
  {
    gui.combo->setCurrentIndex(comboIndex);
  }

  this->inputForm->addRow(QString::fromStdString(role) + ":", gui.combo);
}
