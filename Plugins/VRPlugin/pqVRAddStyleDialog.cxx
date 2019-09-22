/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqVRAddStyleDialog.h"
#include "ui_pqVRAddStyleDialog.h"

#include "pqVRConnectionManager.h"
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
#include "pqVRPNConnection.h"
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
#include "pqVRUIConnection.h"
#endif

#include "vtkNew.h"
#include "vtkSMProxy.h"
#include "vtkStringList.h"
#include "vtkVRInteractorStyle.h"

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
  vtkVRInteractorStyle* Style;

  enum InputType
  {
    Analog = 0,
    Button,
    Tracker
  };

  struct InputGui
  {
    InputType type;
    vtkStdString role;
    QComboBox* combo;
  };

  void AddInput(
    QWidget* parent, InputType type, const vtkStdString& role, const vtkStdString& name);
  QList<InputGui> Inputs;

  QStringList AnalogNames;
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

  std::map<std::string, std::string> analogs;
  std::map<std::string, std::string> buttons;
  std::map<std::string, std::string> trackers;

  foreach (const QString& connName, mgr->connectionNames())
  {
    analogs.clear();
    buttons.clear();
    trackers.clear();
    // Lookup connection
    bool found = false;
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
    if (pqVRPNConnection* conn = mgr->GetVRPNConnection(connName))
    {
      analogs = conn->analogMap();
      buttons = conn->buttonMap();
      trackers = conn->trackerMap();
      found = true;
    }
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
    if (pqVRUIConnection* conn = mgr->GetVRUIConnection(connName))
    {
      if (!found)
      {
        analogs = conn->analogMap();
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
    for (it = analogs.begin(), it_end = analogs.end(); it != it_end; ++it)
    {
      this->Internals->AnalogNames.append(
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

  std::sort(this->Internals->AnalogNames.begin(), this->Internals->AnalogNames.end());
  std::sort(this->Internals->ButtonNames.begin(), this->Internals->ButtonNames.end());
  std::sort(this->Internals->TrackerNames.begin(), this->Internals->TrackerNames.end());
}

//-----------------------------------------------------------------------------
pqVRAddStyleDialog::~pqVRAddStyleDialog()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqVRAddStyleDialog::setInteractorStyle(vtkVRInteractorStyle* style, const QString& name)
{
  this->Internals->Style = style;
  this->Internals->infoLabel->setText(QString("Configuring style %1.").arg(name));

  // Create gui
  vtkNew<vtkStringList> roles;
  style->GetAnalogRoles(roles.GetPointer());
  for (int i = 0; i < roles->GetNumberOfStrings(); ++i)
  {
    vtkStdString role(roles->GetString(i));
    vtkStdString analogName(style->GetAnalogName(role));
    this->Internals->AddInput(this, pqInternals::Analog, role, analogName);
  }
  style->GetButtonRoles(roles.GetPointer());
  for (int i = 0; i < roles->GetNumberOfStrings(); ++i)
  {
    vtkStdString role(roles->GetString(i));
    vtkStdString buttonName(style->GetButtonName(role));
    this->Internals->AddInput(this, pqInternals::Button, role, buttonName);
  }
  style->GetTrackerRoles(roles.GetPointer());
  for (int i = 0; i < roles->GetNumberOfStrings(); ++i)
  {
    vtkStdString role(roles->GetString(i));
    vtkStdString trackerName(style->GetTrackerName(role));
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

  foreach (const pqInternals::InputGui& gui, this->Internals->Inputs)
  {
    const vtkStdString& role = gui.role;
    const vtkStdString& name = gui.combo->currentText().toStdString();
    switch (gui.type)
    {
      case pqInternals::Analog:
        this->Internals->Style->SetAnalogName(role, name);
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
  QWidget* parent, InputType type, const vtkStdString& role, const vtkStdString& name)
{
  this->Inputs.push_back(InputGui());
  InputGui& gui = this->Inputs.back();

  gui.type = type;
  gui.role = role;
  gui.combo = new QComboBox(parent);
  switch (type)
  {
    case Analog:
      gui.combo->addItems(this->AnalogNames);
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
