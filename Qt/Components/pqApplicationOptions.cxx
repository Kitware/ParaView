/*=========================================================================

   Program: ParaView
   Module:    pqApplicationOptions.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/


#include "pqApplicationOptions.h"
#include "ui_pqApplicationOptions.h"

#include "pqApplicationCore.h"
#include "pqViewModuleInterface.h"
#include "pqSettings.h"
#include "pqPluginManager.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqObjectInspectorWidget.h"

#include <QDoubleValidator>

class pqApplicationOptions::pqInternal 
  : public Ui::pqApplicationOptions
{
public:
};


//----------------------------------------------------------------------------
pqApplicationOptions::pqApplicationOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);
  QDoubleValidator* validator = new QDoubleValidator(this->Internal->HeartBeatTimeout);
  validator->setDecimals(2);
  this->Internal->HeartBeatTimeout->setValidator(validator);
  
  this->Internal->DefaultViewType->addItem("None", "None");
  // Get available view types.
  QObjectList ifaces =
    pqApplicationCore::instance()->getPluginManager()->interfaces();
  foreach(QObject* iface, ifaces)
    {
    pqViewModuleInterface* vi = qobject_cast<pqViewModuleInterface*>(iface);
    if(vi)
      {
      QStringList viewtypes = vi->viewTypes();
      QStringList::iterator iter;
      for(iter = viewtypes.begin(); iter != viewtypes.end(); ++iter)
        {
        if ((*iter) == "TableView")
          {
          // Ignore this view for now.
          continue;
          }

        this->Internal->DefaultViewType->addItem(
          vi->viewTypeName(*iter), *iter);
        }
      }
    } 

  // start fresh
  this->resetChanges();

  // enable the apply button when things are changed
  QObject::connect(this->Internal->DefaultViewType,
                  SIGNAL(currentIndexChanged(int)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->HeartBeatTimeout,
                  SIGNAL(textChanged(const QString&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->AutoAccept,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->CrashRecovery,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
}

//-----------------------------------------------------------------------------
pqApplicationOptions::~pqApplicationOptions()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqApplicationOptions::setPage(const QString &page)
{
  int count = this->Internal->stackedWidget->count();
  for(int i=0; i<count; i++)
    {
    if(this->Internal->stackedWidget->widget(i)->objectName() == page)
      {
      this->Internal->stackedWidget->setCurrentIndex(i);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
QStringList pqApplicationOptions::getPageList()
{
  QStringList pages;

  int count = this->Internal->stackedWidget->count();
  for(int i=0; i<count; i++)
    {
    pages << this->Internal->stackedWidget->widget(i)->objectName();
    }
  return pages;
}
  
//-----------------------------------------------------------------------------
void pqApplicationOptions::applyChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("defaultViewType", 
    this->Internal->DefaultViewType->itemData(
      this->Internal->DefaultViewType->currentIndex()));
  pqServer::setHeartBeatTimeoutSetting(static_cast<int>(
      this->Internal->HeartBeatTimeout->text().toDouble()*60*1000));

  bool autoAccept = this->Internal->AutoAccept->isChecked();
  settings->setValue("autoAccept", autoAccept);
  pqObjectInspectorWidget::setAutoAccept(autoAccept);

  bool crashRecovery = this->Internal->CrashRecovery->isChecked();
  settings->setValue("crashRecovery",crashRecovery);
}

//-----------------------------------------------------------------------------
void pqApplicationOptions::resetChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();

  QString curView = settings->value("defaultViewType",
      pqRenderView::renderViewType()).toString();
  int index = this->Internal->DefaultViewType->findData(curView);
  index = (index==-1)? 0 : index;
  this->Internal->DefaultViewType->setCurrentIndex(index);

  this->Internal->HeartBeatTimeout->setText(
    QString("%1").arg(pqServer::getHeartBeatTimeoutSetting()/(60.0*1000), 0, 'f', 2));

  this->Internal->AutoAccept->setChecked(
    settings->value("autoAccept", false).toBool());

  this->Internal->CrashRecovery->setChecked(
    settings->value("crashRecovery", false).toBool());
}

