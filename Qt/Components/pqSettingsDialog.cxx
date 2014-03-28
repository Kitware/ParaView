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
#include "pqServer.h"
#include "pqSettings.h"
#include "vtkNew.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkPVXMLElement.h"

#include <QMap>
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
};

//-----------------------------------------------------------------------------
pqSettingsDialog::pqSettingsDialog(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f),
  Internals (new pqSettingsDialog::pqInternals())
{
  this->Internals->Ui.setupUi(this);

  Ui::SettingsDialog &ui = this->Internals->Ui;
  this->connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), SLOT(clicked(QAbstractButton*)));
  this->connect(this, SIGNAL(accepted()), SLOT(onAccept()));

  this->connect(ui.tabWidget, SIGNAL(currentChanged(int)),
                this, SLOT(onTabIndexChanged(int)));

  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSession(
    pqActiveObjects::instance().activeServer()->session());
  iter->SetModeToOneGroup();
  for (iter->Begin("options"); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxy* proxy = iter->GetProxy();
    if (proxy)
      {
      QScrollArea *scrollArea = new QScrollArea(this);
      scrollArea->setObjectName("ScrollArea");
      scrollArea->setWidgetResizable(true);
      scrollArea->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
      scrollArea->setFrameShape(QFrame::NoFrame);

      QWidget* container = new QWidget(scrollArea);
      container->setObjectName("Container");
      container->setContentsMargins(0, 3, 6, 0);

      QVBoxLayout* vbox = new QVBoxLayout(container);
      vbox->setMargin(0);
      vbox->setSpacing(0);

      pqProxyWidget* widget = new pqProxyWidget(proxy, container);
      widget->setObjectName(iter->GetKey());
      widget->setApplyChangesImmediately(false);
      widget->setView(NULL);

      widget->connect(this, SIGNAL(accepted()), SLOT(apply()));
      widget->connect(this, SIGNAL(rejected()), SLOT(reset()));
      vbox->addWidget(widget);

      QSpacerItem* spacer = new QSpacerItem(0, 0,QSizePolicy::Fixed,
        QSizePolicy::MinimumExpanding);
      vbox->addItem(spacer);

      scrollArea->setWidget(container);
      // show panel widgets
      widget->updatePanel();

      // FIXME: add ability to enable/disable buttons if changes are available.
      int tabIndex = ui.tabWidget->addTab(proxy->GetXMLLabel());
      int stackIndex = ui.stackedWidget->addWidget(scrollArea);
      this->Internals->TabToStackedWidgets[tabIndex] = stackIndex;
      }
    }    

  // After all the tabs are set up, select the first
  this->onTabIndexChanged(0);
}

//-----------------------------------------------------------------------------
pqSettingsDialog::~pqSettingsDialog()
{
  delete this->Internals;
  this->Internals = NULL;
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
void pqSettingsDialog::onAccept()
{
  // if there are any properties that needed to save their values in QSettings,
  // do that.
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSession(
    pqActiveObjects::instance().activeServer()->session());
  iter->SetModeToOneGroup();
  for (iter->Begin("options"); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxy* proxy = iter->GetProxy();
    vtkSmartPointer<vtkSMPropertyIterator> iter2;
    iter2.TakeReference(proxy->NewPropertyIterator());
    for (iter2->Begin(); !iter2->IsAtEnd(); iter2->Next())
      {
      vtkSMProperty* smproperty = iter2->GetProperty();
      if (smproperty && smproperty->GetHints() &&
        smproperty->GetHints()->FindNestedElementByName("SaveInQSettings"))
        {
        QString key = QString("%1.%2").arg(iter->GetKey()).arg(iter2->GetKey());
        this->saveInQSettings(key.toAscii().data(), smproperty);
        }
      }
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
