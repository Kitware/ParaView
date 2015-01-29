/*=========================================================================

   Program: ParaView
   Module:  pqSettingsDialog.cxx

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
#include "pqSettingsDialog.h"
#include "ui_pqSettingsDialog.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqProxyWidget.h"
#include "pqSearchBox.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqUndoStack.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"

#include <QKeyEvent>
#include <QMap>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSpacerItem>
#include <QVBoxLayout>

class pqSettingsDialog::pqInternals
{
public:
  Ui::SettingsDialog Ui;

  // Map from tab indices to stack widget indices. This is needed because there
  // are more widgets in the stacked widgets than just what we add.
  QMap<int, int> TabToStackedWidgets;

  QPointer<pqServer> Server;
};

bool pqSettingsDialog::ShowRestartRequired = false;

//-----------------------------------------------------------------------------
pqSettingsDialog::pqSettingsDialog(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f),
  Internals (new pqSettingsDialog::pqInternals())
{
  Ui::SettingsDialog &ui = this->Internals->Ui;
  ui.setupUi(this);
  ui.tabBar->setDocumentMode(false);
  ui.tabBar->setDrawBase(false);
  ui.tabBar->setExpanding(false);
  ui.tabBar->setUsesScrollButtons(true);

  // Hide restart message
  ui.restartRequiredLabel->setVisible(pqSettingsDialog::ShowRestartRequired);

  QList<vtkSMProxy*> proxies_to_show;

  pqServer* server = pqActiveObjects::instance().activeServer();
  this->Internals->Server = server;

  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSession(server->session());
  iter->SetModeToOneGroup();
  for (iter->Begin("settings"); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxy* proxy = iter->GetProxy();
    if (proxy)
      {
      proxies_to_show.push_back(proxy);
      }
    }

  // Add color palette.
  if (vtkSMProxy* proxy = server->proxyManager()->GetProxy("global_properties", "ColorPalette"))
    {
    proxies_to_show.push_back(proxy);
    }

  foreach (vtkSMProxy* proxy, proxies_to_show)
    {
    QString proxyName = proxy->GetXMLName();

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setObjectName(QString("ScrollArea%1").arg(proxyName));
    scrollArea->setWidgetResizable(true);
    scrollArea->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* container = new QWidget(scrollArea);
    container->setObjectName("Container");
    container->setContentsMargins(6, 0, 6, 0);

    QVBoxLayout* vbox = new QVBoxLayout(container);
    vbox->setMargin(0);
    vbox->setSpacing(0);

    pqProxyWidget* widget = new pqProxyWidget(proxy, container);
    widget->setObjectName("ProxyWidget");
    widget->setApplyChangesImmediately(false);
    widget->setView(NULL);

    widget->connect(this, SIGNAL(accepted()), SLOT(apply()));
    widget->connect(this, SIGNAL(rejected()), SLOT(reset()));
    this->connect(widget, SIGNAL(restartRequired()), SLOT(showRestartRequiredMessage()));
    vbox->addWidget(widget);

    QSpacerItem* spacer = new QSpacerItem(0, 0,QSizePolicy::Fixed,
      QSizePolicy::MinimumExpanding);
    vbox->addItem(spacer);

    scrollArea->setWidget(container);
    // show panel widgets
    widget->updatePanel();

    int tabIndex = ui.tabBar->addTab(proxy->GetXMLLabel());
    int stackIndex = ui.stackedWidget->addWidget(scrollArea);
    this->Internals->TabToStackedWidgets[tabIndex] = stackIndex;

    this->connect(widget, SIGNAL(changeAvailable()), SLOT(onChangeAvailable()));
    widget->connect(this, SIGNAL(filterWidgets(bool, QString)), SLOT(filterWidgets(bool, QString)));
    }

  // Disable some buttons to start
  ui.buttonBox->button(QDialogButtonBox::Reset)->setEnabled(false);
  ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

  this->connect(ui.buttonBox->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked()),
                SLOT(onRestoreDefaults()));
  this->connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), SLOT(clicked(QAbstractButton*)));
  this->connect(this, SIGNAL(accepted()), SLOT(onAccepted()));
  this->connect(this, SIGNAL(rejected()), SLOT(onRejected()));
  this->connect(ui.tabBar, SIGNAL(currentChanged(int)), this, SLOT(onTabIndexChanged(int)));

  this->connect(ui.SearchBox, SIGNAL(advancedSearchActivated(bool)), SLOT(filterPanelWidgets()));
  this->connect(ui.SearchBox, SIGNAL(textChanged(QString)), SLOT(filterPanelWidgets()));

  // After all the tabs are set up, select the first
  this->onTabIndexChanged(0);

  this->filterPanelWidgets();

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smmodel, SIGNAL(serverRemoved(pqServer*)), SLOT(serverRemoved(pqServer*)));
}

//-----------------------------------------------------------------------------
pqSettingsDialog::~pqSettingsDialog()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::serverRemoved(pqServer* server)
{
  // BUG #14957: Close this dialog if the server session closes.
  if (this->Internals->Server == server)
    {
    this->close();
    }
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::clicked(QAbstractButton *button)
{
  Ui::SettingsDialog &ui = this->Internals->Ui;
  QDialogButtonBox::ButtonRole role = ui.buttonBox->buttonRole(button);
  switch (role)
    {
  case QDialogButtonBox::AcceptRole:
  case QDialogButtonBox::ApplyRole:
    emit this->accepted();
    break;

  case QDialogButtonBox::ResetRole:
  case QDialogButtonBox::RejectRole:
    emit this->rejected();
    break;
  default:
    break;
    }
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::onAccepted()
{
  // If there are any properties that needed to save their values in QSettings,
  // do that. Otherwise, save to the vtkSMSettings singleton.
  vtkSMSettings * settings = vtkSMSettings::GetInstance();
  pqServer* server = pqActiveObjects::instance().activeServer();
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSession(server->session());
  iter->SetModeToOneGroup();
  for (iter->Begin("settings"); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxy* proxy = iter->GetProxy();
    settings->SetProxySettings(proxy);
    vtkSmartPointer<vtkSMPropertyIterator> iter2;
    iter2.TakeReference(proxy->NewPropertyIterator());
    for (iter2->Begin(); !iter2->IsAtEnd(); iter2->Next())
      {
      vtkSMProperty* smproperty = iter2->GetProperty();
      if (smproperty && smproperty->GetHints() &&
        smproperty->GetHints()->FindNestedElementByName("SaveInQSettings"))
        {
        QString key = QString("%1.%2").arg(iter->GetKey()).arg(iter2->GetKey());
        this->saveInQSettings(key.toLatin1().data(), smproperty);
        }
      }
    }

  // Save color palette settings
  vtkSMProxy* paletteProxy = server->proxyManager()->GetProxy("global_properties", "ColorPalette");
  if (paletteProxy)
    {
    settings->SetProxySettings(paletteProxy);
    }

  // Disable buttons
  Ui::SettingsDialog &ui = this->Internals->Ui;
  ui.buttonBox->button(QDialogButtonBox::Reset)->setEnabled(false);
  ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

  // In theory, the above changes are undo-redo able, the only things that's not
  // undo-able is the "serialized" values. Hence we just clear the undo stack.
  CLEAR_UNDO_STACK();

  // Render all views.
  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::onRejected()
{
  // Disable buttons
  Ui::SettingsDialog &ui = this->Internals->Ui;
  ui.buttonBox->button(QDialogButtonBox::Reset)->setEnabled(false);
  ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::onRestoreDefaults()
{
  pqServer* server = pqActiveObjects::instance().activeServer();  
  vtkSMSession * session = server->session();

  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSession(session);
  iter->SetModeToOneGroup();
  for (iter->Begin("settings"); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxy* proxy = iter->GetProxy();
    if (proxy)
      {
      proxy->ResetPropertiesToXMLDefaults();
      }
    }

  vtkSMProxy* paletteProxy = server->proxyManager()->GetProxy("global_properties", "ColorPalette");
  if (paletteProxy)
    {
    paletteProxy->ResetPropertiesToXMLDefaults();
    }
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::onTabIndexChanged(int index)
{
  int stackWidgetIndex = this->Internals->TabToStackedWidgets[index];
  Ui::SettingsDialog &ui = this->Internals->Ui;
  ui.stackedWidget->setCurrentIndex(stackWidgetIndex);
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::filterPanelWidgets()
{
  Ui::SettingsDialog &ui = this->Internals->Ui;
  emit this->filterWidgets(
    ui.SearchBox->isAdvancedSearchActive(), ui.SearchBox->text());
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::onChangeAvailable()
{
  Ui::SettingsDialog &ui = this->Internals->Ui;
  ui.buttonBox->button(QDialogButtonBox::Reset)->setEnabled(true);
  ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::showRestartRequiredMessage()
{
  Ui::SettingsDialog &ui = this->Internals->Ui;
  ui.restartRequiredLabel->setVisible(true);
  pqSettingsDialog::ShowRestartRequired = true;
}

//-----------------------------------------------------------------------------
void pqSettingsDialog::saveInQSettings(
  const char* key, vtkSMProperty* smproperty)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();

  // FIXME: handle all property types. This will only work for single value
  // properties.
  if (smproperty->IsA("vtkSMIntVectorProperty") ||
    smproperty->IsA("vtkSMIdTypeVectorProperty"))
    {
    settings->setValue(key, vtkSMPropertyHelper(smproperty).GetAsInt());
    }
  else if (smproperty->IsA("vtkSMDoubleVectorProperty"))
    {
    settings->setValue(key, vtkSMPropertyHelper(smproperty).GetAsDouble());
    }
  else if (smproperty->IsA("vtkSMStringVectorProperty"))
    {
    settings->setValue(key, vtkSMPropertyHelper(smproperty).GetAsString());
    }
}
