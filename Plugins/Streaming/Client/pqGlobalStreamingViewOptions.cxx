/*=========================================================================

   Program: ParaView
   Module:    pqGlobalStreamingViewOptions.cxx

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


#include "pqGlobalStreamingViewOptions.h"
#include "ui_pqGlobalStreamingViewOptions.h"

#include "pqApplicationCore.h"
#include "pqViewModuleInterface.h"
#include "pqSettings.h"
#include "pqPluginManager.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqObjectInspectorWidget.h"

#include <QDoubleValidator>

class pqGlobalStreamingViewOptions::pqInternal 
  : public Ui::pqGlobalStreamingViewOptions
{
public:
};


//----------------------------------------------------------------------------
pqGlobalStreamingViewOptions::pqGlobalStreamingViewOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);


  QIntValidator* sValidator = new QIntValidator(this->Internal->StreamedPasses);
  this->Internal->StreamedPasses->setValidator(sValidator);

  QIntValidator* cValidator = new QIntValidator(this->Internal->PieceCacheLimit);
  this->Internal->PieceCacheLimit->setValidator(cValidator);

  QIntValidator* rValidator = new QIntValidator(this->Internal->PieceRenderCutoff);
  this->Internal->PieceRenderCutoff->setValidator(rValidator);
  

  // start fresh
  this->resetChanges();
  this->applyChanges();

  // enable the apply button when things are changed
  QObject::connect(this->Internal->StreamedPasses,
                  SIGNAL(textChanged(const QString&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->EnableStreamMessages,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->UseCulling,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->UseViewOrdering,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->PieceCacheLimit,
                  SIGNAL(textChanged(const QString&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->PieceRenderCutoff,
                  SIGNAL(textChanged(const QString&)),
                  this, SIGNAL(changesAvailable()));
}

//-----------------------------------------------------------------------------
pqGlobalStreamingViewOptions::~pqGlobalStreamingViewOptions()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqGlobalStreamingViewOptions::setPage(const QString &page)
{
  if(page == "Streaming View")
    {
    this->Internal->stackedWidget->setCurrentIndex(0);
    }

  QString which = page.section(".", 1, 1);
  int count = this->Internal->stackedWidget->count();
  for(int i=0; i<count; i++)
    {
    if(this->Internal->stackedWidget->widget(i)->objectName() == which)
      {
      this->Internal->stackedWidget->setCurrentIndex(i);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
QStringList pqGlobalStreamingViewOptions::getPageList()
{
  QStringList pages("Streaming View");
  int count = this->Internal->stackedWidget->count();
  if (count > 1)
    {
    for(int i=0; i<count; i++)
      {
      pages << "Streaming View." + this->Internal->stackedWidget->widget(i)->objectName();
      }
    }
  return pages;

}
  
//-----------------------------------------------------------------------------
void pqGlobalStreamingViewOptions::applyChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("streamingView");

  int intSetting;
  bool setting;

  intSetting = this->Internal->StreamedPasses->text().toInt();
  pqServer::setStreamedPasses(intSetting);
  settings->setValue("StreamedPasses", intSetting);

  setting = this->Internal->EnableStreamMessages->isChecked();
  pqServer::setEnableStreamMessages(setting);
  settings->setValue("EnableStreamMessages", setting);

  setting = this->Internal->UseCulling->isChecked();
  pqServer::setUseCulling(setting);
  settings->setValue("UseCulling", setting);

  setting = this->Internal->UseViewOrdering->isChecked();
  pqServer::setUseViewOrdering(setting);
  settings->setValue("UseViewOrdering", setting);

  intSetting = this->Internal->PieceCacheLimit->text().toInt();
  pqServer::setPieceCacheLimit(intSetting);
  settings->setValue("PieceCacheLimit", intSetting);

  intSetting = this->Internal->PieceRenderCutoff->text().toInt();
  pqServer::setPieceRenderCutoff(intSetting);
  settings->setValue("PieceRenderCutoff", intSetting);

  settings->endGroup();
  settings->alertSettingsModified();

  // loop through render views and apply new settings
  /*QList<pqRenderView*> views =
    pqApplicationCore::instance()->getServerManagerModel()->
    findItems<pqRenderView*>();

  foreach(pqRenderView* view, views)
    {
    view->restoreSettings(true);
    }
  */
}

//-----------------------------------------------------------------------------
void pqGlobalStreamingViewOptions::resetChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();

  settings->beginGroup("streamingView");

  QVariant val = settings->value("StreamedPasses", 16);
  this->Internal->StreamedPasses->setText(val.toString());

  val = settings->value("EnableStreamMessages", false);
  this->Internal->EnableStreamMessages->setChecked(val.toBool());

  val = settings->value("UseCulling", true);
  this->Internal->UseCulling->setChecked(val.toBool());

  val = settings->value("UseViewOrdering", true);
  this->Internal->UseViewOrdering->setChecked(val.toBool());

  val = settings->value("PieceCacheLimit", 16);
  this->Internal->PieceCacheLimit->setText(val.toString());

  val = settings->value("PieceRenderCutoff", -1);
  this->Internal->PieceRenderCutoff->setText(val.toString());

  settings->endGroup();

}

