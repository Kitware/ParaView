// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqVRAddConnectionDialog.h"
#include "ui_pqVRAddConnectionDialog.h"

#include "pqVRConnectionManager.h"
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
#include "pqVRPNConnection.h"
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
#include "pqVRUIConnection.h"
#endif

#include <QKeyEvent>
#include <QMessageBox>
#include <QRegExpValidator>

#include <QtCore/QDebug>
#include <QtCore/QPair>
#include <QtCore/QRegExp>
#include <QtCore/QString>

#include <map>

class pqVRAddConnectionDialog::pqInternals : public Ui::VRAddConnectionDialog
{
public:
  enum InputType
  {
    Valuator = 0,
    Button,
    Tracker
  };

  std::map<std::string, std::string> ValuatorMapping;
  std::map<std::string, std::string> ButtonMapping;
  std::map<std::string, std::string> TrackerMapping;

  enum ConnectionType
  {
    None = -1,
    VRPN,
    VRUI
  };

  ConnectionType Type;

  ConnectionType GetSelectedConnectionType()
  {
    if (this->connectionType->currentText() == "VRPN")
    {
      return VRPN;
    }
    else if (this->connectionType->currentText() == "VRUI")
    {
      return VRUI;
    }
    return None;
  }

  bool SetSelectedConnectionType(ConnectionType type)
  {
    switch (type)
    {
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
      case pqInternals::VRPN:
      {
        int ind = this->connectionType->findText("VRPN");
        if (ind == -1)
        {
          return false;
        }
        this->connectionType->setCurrentIndex(ind);
        return true;
      }
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
      case pqInternals::VRUI:
      {
        int ind = this->connectionType->findText("VRUI");
        if (ind == -1)
        {
          return false;
        }
        this->connectionType->setCurrentIndex(ind);
        return true;
      }
#endif
      default:
      case pqInternals::None:
        return false;
    }
  }

#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
  pqVRPNConnection* VRPNConn;
  void updateVRPNConnection();
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
  pqVRUIConnection* VRUIConn;
  void updateVRUIConnection();
#endif

  /// Given an entry in the listWidget, return a pair: id, name
  QPair<QString, QString> parseEntry(const QString&);

  void addInput();
  void removeInput();

  void updateUi();
};

//-----------------------------------------------------------------------------
pqVRAddConnectionDialog::pqVRAddConnectionDialog(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqInternals())
{
  this->Internals->setupUi(this);
  this->Internals->Type = pqInternals::None;
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
  this->Internals->VRPNConn = nullptr;
  this->Internals->connectionType->addItem("VRPN");
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
  this->Internals->VRUIConn = nullptr;
  this->Internals->connectionType->addItem("VRUI");
#endif
  this->connectionTypeChanged();

  // Restrict input in some line edits
  QRegExpValidator* connNameValidator = new QRegExpValidator(QRegExp("[0-9a-zA-Z]+"), this);
  QRegExpValidator* addressValidator =
    new QRegExpValidator(QRegExp("([0-9a-zA-Z.]+@)?([a-zA-Z]+://)?[0-9a-zA-Z.:]+"), this);
  QRegExpValidator* inputIdValidator = new QRegExpValidator(QRegExp("[0-9]+"), this);
  QRegExpValidator* inputNameValidator = new QRegExpValidator(QRegExp("[0-9a-zA-Z]+"), this);
  this->Internals->connectionName->setValidator(connNameValidator);
  this->Internals->connectionAddress->setValidator(addressValidator);
  this->Internals->inputId->setValidator(inputIdValidator);
  this->Internals->inputName->setValidator(inputNameValidator);

  connect(this->Internals->insertInput, SIGNAL(clicked()), this, SLOT(addInput()));
  connect(this->Internals->eraseInput, SIGNAL(clicked()), this, SLOT(removeInput()));
  connect(this->Internals->connectionType, SIGNAL(currentIndexChanged(int)), this,
    SLOT(connectionTypeChanged()));
}

//-----------------------------------------------------------------------------
pqVRAddConnectionDialog::~pqVRAddConnectionDialog()
{
  delete this->Internals;
}

#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::setConnection(pqVRPNConnection* conn)
{
  this->Internals->VRPNConn = conn;
  this->Internals->Type = pqInternals::VRPN;
  this->Internals->connectionName->setText(QString::fromStdString(conn->name()));
  this->Internals->connectionAddress->setText(QString::fromStdString(conn->address()));
  this->Internals->SetSelectedConnectionType(pqInternals::VRPN);
  this->Internals->connectionType->setEnabled(false);
  this->Internals->ValuatorMapping = conn->valuatorMap();
  this->Internals->ButtonMapping = conn->buttonMap();
  this->Internals->TrackerMapping = conn->trackerMap();
  this->Internals->updateUi();
}

//-----------------------------------------------------------------------------
pqVRPNConnection* pqVRAddConnectionDialog::getVRPNConnection()
{
  if (this->Internals->Type == pqInternals::VRPN)
  {
    return this->Internals->VRPNConn;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
bool pqVRAddConnectionDialog::isVRPN()
{
  return this->Internals->Type == pqInternals::VRPN;
}
#endif

#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::setConnection(pqVRUIConnection* conn)
{
  this->Internals->VRUIConn = conn;
  this->Internals->Type = pqInternals::VRUI;
  this->Internals->connectionName->setText(QString::fromStdString(conn->name()));
  this->Internals->connectionAddress->setText(QString::fromStdString(conn->address()));
  this->Internals->connectionPort->setValue(QString::fromStdString(conn->port()).toInt());
  this->Internals->SetSelectedConnectionType(pqInternals::VRUI);
  this->Internals->connectionType->setEnabled(false);
  this->Internals->ValuatorMapping = conn->valuatorMap();
  this->Internals->ButtonMapping = conn->buttonMap();
  this->Internals->TrackerMapping = conn->trackerMap();
  this->Internals->updateUi();
}

//-----------------------------------------------------------------------------
pqVRUIConnection* pqVRAddConnectionDialog::getVRUIConnection()
{
  if (this->Internals->Type == pqInternals::VRUI)
  {
    return this->Internals->VRUIConn;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
bool pqVRAddConnectionDialog::isVRUI()
{
  return this->Internals->Type == pqInternals::VRUI;
}
#endif

//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::updateConnection()
{
  switch (this->Internals->Type)
  {
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
    case pqInternals::VRPN:
      this->Internals->updateVRPNConnection();
      return;
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
    case pqInternals::VRUI:
      this->Internals->updateVRUIConnection();
      return;
#endif
    default:
    case pqInternals::None:
      switch (this->Internals->GetSelectedConnectionType())
      {
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
        case pqInternals::VRPN:
          this->Internals->updateVRPNConnection();
          return;
#endif
#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
        case pqInternals::VRUI:
          this->Internals->updateVRUIConnection();
          return;
#endif
        default:
          qWarning() << "Cannot create connection...unsupported connection type.";
          return;
      }
  }
}

void pqVRAddConnectionDialog::accept()
{
  if (this->Internals->connectionName->text().isEmpty())
  {
    QMessageBox::critical(this, tr("Missing connection name"), tr("Connection name is not set!"));
    return;
  }
  if (this->Internals->connectionAddress->text().isEmpty())
  {
    QMessageBox::critical(
      this, tr("Missing connection adress"), tr("Connection address is not set!"));
    return;
  }

  this->Superclass::accept();
}

//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::keyPressEvent(QKeyEvent* event)
{
  // Disable the default behavior of clicking "Ok" when enter is pressed
  if (!event->modifiers() ||
    (event->modifiers() & Qt::KeypadModifier && event->key() == Qt::Key_Enter))
  {
    switch (event->key())
    {
      case Qt::Key_Enter:
      case Qt::Key_Return:
        if (this->Internals->insertInput->hasFocus())
        {
          this->Internals->insertInput->click();
          event->accept();
          return;
        }
        return;
    }
  }
  QDialog::keyPressEvent(event);
}

//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::addInput()
{
  if (this->Internals->inputName->text().isEmpty())
  {
    QMessageBox::critical(this, tr("Missing input name"), tr("Input name is not set!"));
    return;
  }
  if (this->Internals->inputId->text().isEmpty())
  {
    QMessageBox::critical(this, tr("Missing input id"), tr("Input id is not set!"));
    return;
  }

  this->Internals->addInput();
}

//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::removeInput()
{
  this->Internals->removeInput();
}

//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::connectionTypeChanged()
{
  switch (this->Internals->GetSelectedConnectionType())
  {
    case pqInternals::VRPN:
      this->Internals->portLabel->hide();
      this->Internals->connectionPort->hide();
      break;
    case pqInternals::VRUI:
      this->Internals->portLabel->show();
      this->Internals->connectionPort->show();
      break;
    default:
      break;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------pqInternals methods-----------------------------
//-----------------------------------------------------------------------------

#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRPN
//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::pqInternals::updateVRPNConnection()
{
  if (!this->VRPNConn)
  {
    this->VRPNConn = new pqVRPNConnection(pqVRConnectionManager::instance());
    this->Type = VRPN;
  }

  this->VRPNConn->setName(this->connectionName->text().toStdString());
  this->VRPNConn->setAddress(this->connectionAddress->text().toStdString());
  this->VRPNConn->setValuatorMap(this->ValuatorMapping);
  this->VRPNConn->setButtonMap(this->ButtonMapping);
  this->VRPNConn->setTrackerMap(this->TrackerMapping);
}
#endif

#if PARAVIEW_PLUGIN_CAVEInteraction_USE_VRUI
//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::pqInternals::updateVRUIConnection()
{
  if (!this->VRUIConn)
  {
    this->VRUIConn = new pqVRUIConnection(pqVRConnectionManager::instance());
    this->Type = VRUI;
  }

  this->VRUIConn->setName(this->connectionName->text().toStdString());
  this->VRUIConn->setAddress(this->connectionAddress->text().toStdString());
  this->VRUIConn->setPort(QString::number(this->connectionPort->value()).toStdString());
  this->VRUIConn->setValuatorMap(this->ValuatorMapping);
  this->VRUIConn->setButtonMap(this->ButtonMapping);
  this->VRUIConn->setTrackerMap(this->TrackerMapping);
}
#endif

//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::pqInternals::updateUi()
{
  this->listWidget->clear();
  std::map<std::string, std::string>::const_iterator it;
  std::map<std::string, std::string>::const_iterator it_end;
  for (it = this->ValuatorMapping.begin(), it_end = this->ValuatorMapping.end(); it != it_end; ++it)
  {
    this->listWidget->addItem(QString("%1: %2")
                                .arg(QString::fromStdString(it->first))
                                .arg(QString::fromStdString(it->second)));
  }
  for (it = this->ButtonMapping.begin(), it_end = this->ButtonMapping.end(); it != it_end; ++it)
  {
    this->listWidget->addItem(QString("%1: %2")
                                .arg(QString::fromStdString(it->first))
                                .arg(QString::fromStdString(it->second)));
  }
  for (it = this->TrackerMapping.begin(), it_end = this->TrackerMapping.end(); it != it_end; ++it)
  {
    this->listWidget->addItem(QString("%1: %2")
                                .arg(QString::fromStdString(it->first))
                                .arg(QString::fromStdString(it->second)));
  }
}

//-----------------------------------------------------------------------------
QPair<QString, QString> pqVRAddConnectionDialog::pqInternals::parseEntry(const QString& entry)
{
  QPair<QString, QString> result;
  QRegExp regexp("([^:]+): (.+)");
  if (regexp.indexIn(entry) < 0)
  {
    return result;
  }

  result.first = regexp.cap(1);
  result.second = regexp.cap(2);
  return result;
}

//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::pqInternals::addInput()
{
  std::map<std::string, std::string>* targetMap;
  const char* idPrefix;
  switch (this->inputType->currentIndex())
  {
    case Valuator:
      targetMap = &this->ValuatorMapping;
      idPrefix = "valuator.";
      break;
    case Button:
      targetMap = &this->ButtonMapping;
      idPrefix = "button.";
      break;
    case Tracker:
      targetMap = &this->TrackerMapping;
      idPrefix = "tracker.";
      break;
    default:
      qWarning() << "Invalid input type! This shouldn't happen...";
      return;
  }

  std::string id = this->inputId->text().prepend(idPrefix).toStdString();
  (*targetMap)[id] = this->inputName->text().toStdString();
  this->updateUi();
}

//-----------------------------------------------------------------------------
void pqVRAddConnectionDialog::pqInternals::removeInput()
{
  bool changed = false;
  Q_FOREACH (QListWidgetItem* item, this->listWidget->selectedItems())
  {
    QString id = this->parseEntry(item->text()).first;
    if (id.isEmpty())
    {
      continue;
    }

    if (id.startsWith("valuator."))
    {
      this->ValuatorMapping.erase(id.toStdString());
    }
    else if (id.startsWith("button."))
    {
      this->ButtonMapping.erase(id.toStdString());
    }
    else if (id.startsWith("tracker."))
    {
      this->TrackerMapping.erase(id.toStdString());
    }

    changed = true;
  }

  if (changed)
  {
    this->updateUi();
  }
}
