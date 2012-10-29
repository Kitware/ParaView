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
#ifdef PARAVIEW_USE_VRPN
#include "pqVRPNConnection.h"
#endif
#ifdef PARAVIEW_USE_VRUI
#include "pqVRUIConnection.h"
#endif

#include "vtkSMProxy.h"
#include "vtkVRInteractorStyle.h"

#include <QtCore/QDebug>
#include <QtCore/QStringList>

#include <map>
#include <string>

class pqVRAddStyleDialog::pqInternals : public Ui::VRAddStyleDialog
{
public:
  bool CanConfigure;
  vtkVRInteractorStyle *Style;

  QStringList AnalogInputs;
  QStringList ButtonInputs;
  QStringList TrackerInputs;
};

//-----------------------------------------------------------------------------
pqVRAddStyleDialog::pqVRAddStyleDialog(QWidget* parentObject,
  Qt::WindowFlags f)
  : Superclass(parentObject, f), Internals(new pqInternals())
{
  this->Internals->setupUi(this);
  this->Internals->CanConfigure = false;

  // Populate input lists
  pqVRConnectionManager *mgr = pqVRConnectionManager::instance();

  std::map<std::string, std::string> analogs;
  std::map<std::string, std::string> buttons;
  std::map<std::string, std::string> trackers;

  foreach (const QString &connName, mgr->connectionNames())
    {
    analogs.clear();
    buttons.clear();
    trackers.clear();
    // Lookup connection
    bool found = false;
#ifdef PARAVIEW_USE_VRPN
    if (pqVRPNConnection *conn = mgr->GetVRPNConnection(connName))
      {
      analogs = conn->analogMap();
      buttons = conn->buttonMap();
      trackers = conn->trackerMap();
      found = true;
      }
#endif
#ifdef PARAVIEW_USE_VRUI
    if (pqVRUIConnection *conn = mgr->GetVRUIConnection(connName))
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
      this->Internals->AnalogInputs.append(
            QString("%1.%2").arg(connName)
            .arg(QString::fromStdString(it->second)));
      }
    for (it = buttons.begin(), it_end = buttons.end(); it != it_end; ++it)
      {
      this->Internals->ButtonInputs.append(
            QString("%1.%2").arg(connName)
            .arg(QString::fromStdString(it->second)));
      }
    for (it = trackers.begin(), it_end = trackers.end(); it != it_end; ++it)
      {
      this->Internals->TrackerInputs.append(
            QString("%1.%2").arg(connName)
            .arg(QString::fromStdString(it->second)));
      }
    }

  qSort(this->Internals->AnalogInputs);
  qSort(this->Internals->ButtonInputs);
  qSort(this->Internals->TrackerInputs);

  this->Internals->analogCombo->addItems(this->Internals->AnalogInputs);
  this->Internals->buttonCombo->addItems(this->Internals->ButtonInputs);
  this->Internals->trackerCombo->addItems(this->Internals->TrackerInputs);
}

//-----------------------------------------------------------------------------
pqVRAddStyleDialog::~pqVRAddStyleDialog()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqVRAddStyleDialog::setInteractorStyle(vtkVRInteractorStyle *style,
                                            const QString &name)
{
  this->Internals->Style = style;
  this->Internals->infoLabel->setText(
        QString("Configuring style %1.").arg(name));

  bool needsAnalog = style->GetNeedsAnalog();
  bool needsButton = style->GetNeedsButton();
  bool needsTracker = style->GetNeedsTracker();

  QString analog(style->GetAnalogName());
  QString button(style->GetButtonName());
  QString tracker(style->GetTrackerName());

  this->Internals->analogLabel->setVisible(needsAnalog);
  this->Internals->analogCombo->setVisible(needsAnalog);
  this->Internals->buttonLabel->setVisible(needsButton);
  this->Internals->buttonCombo->setVisible(needsButton);
  this->Internals->trackerLabel->setVisible(needsTracker);
  this->Internals->trackerCombo->setVisible(needsTracker);

  int analogIndex = this->Internals->analogCombo->findText(analog);
  if (analogIndex != -1)
    {
    this->Internals->analogCombo->setCurrentIndex(analogIndex);
    }

  int buttonIndex = this->Internals->buttonCombo->findText(button);
  if (buttonIndex != -1)
    {
    this->Internals->buttonCombo->setCurrentIndex(buttonIndex);
    }

  int trackerIndex = this->Internals->trackerCombo->findText(tracker);
  if (trackerIndex != -1)
    {
    this->Internals->trackerCombo->setCurrentIndex(trackerIndex);
    }

  this->Internals->CanConfigure = needsAnalog || needsButton || needsTracker;
}

//-----------------------------------------------------------------------------
void pqVRAddStyleDialog::updateInteractorStyle()
{
  if (!this->Internals->CanConfigure || !this->Internals->Style)
    {
    return;
    }

  if (this->Internals->Style->GetNeedsAnalog())
    {
    QByteArray analog =
        this->Internals->analogCombo->currentText().toLocal8Bit();
    this->Internals->Style->SetAnalogName(analog.data());
    }
  if (this->Internals->Style->GetNeedsButton())
    {
    QByteArray button =
        this->Internals->buttonCombo->currentText().toLocal8Bit();
    this->Internals->Style->SetButtonName(button.data());
    }
  if (this->Internals->Style->GetNeedsTracker())
    {
    QByteArray tracker =
        this->Internals->trackerCombo->currentText().toLocal8Bit();
    this->Internals->Style->SetTrackerName(tracker.data());
    }
}

//-----------------------------------------------------------------------------
bool pqVRAddStyleDialog::isConfigurable()
{
  return this->Internals->CanConfigure;
}
