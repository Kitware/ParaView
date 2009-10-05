/*=========================================================================

   Program: ParaView
   Module:    pqGlobalAdaptiveViewOptions.cxx

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


#include "pqGlobalAdaptiveViewOptions.h"
#include "ui_pqGlobalAdaptiveViewOptions.h"

#include "pqApplicationCore.h"
#include "pqViewModuleInterface.h"
#include "pqSettings.h"
#include "pqPluginManager.h"
#include "pqRenderView.h"
#include "pqObjectInspectorWidget.h"

#include "vtkSMAdaptiveOptionsProxy.h"
#include "vtkSMIntVectorProperty.h"

#include <QDoubleValidator>
#include <QDebug>

class pqGlobalAdaptiveViewOptions::pqInternal 
  : public Ui::pqGlobalAdaptiveViewOptions
{
public:
};


//----------------------------------------------------------------------------
pqGlobalAdaptiveViewOptions::pqGlobalAdaptiveViewOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal;
  this->Internal->setupUi(this);

  QIntValidator* cValidator = new QIntValidator(this->Internal->PieceCacheLimit);
  this->Internal->PieceCacheLimit->setValidator(cValidator);

  // start fresh
  this->resetChanges();
  this->applyChanges();

  // enable the apply button when things are changed
  QObject::connect(this->Internal->EnableStreamMessages,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->ShowOn,
                  SIGNAL(currentIndexChanged(int)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->UsePrioritization,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->UseViewOrdering,
                  SIGNAL(toggled(bool)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->PieceCacheLimit,
                  SIGNAL(textChanged(const QString&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Height,
                  SIGNAL(textChanged(const QString&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Degree,
                  SIGNAL(textChanged(const QString&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->Rate,
                   SIGNAL(textChanged(const QString&)),
                   this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->MaxSplits,
                  SIGNAL(textChanged(const QString&)),
                  this, SIGNAL(changesAvailable()));

}

//-----------------------------------------------------------------------------
pqGlobalAdaptiveViewOptions::~pqGlobalAdaptiveViewOptions()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqGlobalAdaptiveViewOptions::setPage(const QString &page)
{
  if(page == "Adaptive View")
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
QStringList pqGlobalAdaptiveViewOptions::getPageList()
{
  QStringList pages("Adaptive View");
  int count = this->Internal->stackedWidget->count();
  if (count > 1)
    {
    for(int i=0; i<count; i++)
      {
      pages << "Adaptive View." + this->Internal->stackedWidget->widget(i)->objectName();
      }
    }
  return pages;

}
  
#define QUICKSETVAL(name, val)\
  p = vtkSMIntVectorProperty::SafeDownCast(\
    helper->GetProperty(name));\
  p->SetElement(0, val);

//-----------------------------------------------------------------------------
void pqGlobalAdaptiveViewOptions::applyChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("adaptiveView");

  int intSetting;
  bool boolSetting;

  vtkSMAdaptiveOptionsProxy* helper =
    vtkSMAdaptiveOptionsProxy::GetProxy();
  if (!helper)
    {
    qCritical() << "Trying to apply changes to adaptive settings but "
                << "adaptive helper proxy is null.";
    }

  vtkSMIntVectorProperty *p;

  boolSetting = this->Internal->EnableStreamMessages->isChecked();
  QUICKSETVAL("EnableStreamMessages", (boolSetting?1:0));
  settings->setValue("EnableStreamMessages", boolSetting);

  boolSetting = this->Internal->UsePrioritization->isChecked();
  QUICKSETVAL("UsePrioritization", (boolSetting?1:0));
  settings->setValue("UsePrioritization", boolSetting);
  if (!boolSetting)
    {
    this->Internal->UseViewOrdering->setChecked(false);
    }

  boolSetting = this->Internal->UseViewOrdering->isChecked();
  QUICKSETVAL("UseViewOrdering", (boolSetting?1:0));
  settings->setValue("UseViewOrdering", boolSetting);

  intSetting = this->Internal->ShowOn->currentIndex();
  QUICKSETVAL("ShowOn", intSetting);
  settings->setValue("ShowOn", intSetting);

  intSetting = this->Internal->PieceCacheLimit->text().toInt();
  if (intSetting < -1)
    {
    intSetting = -1;
    }
  QUICKSETVAL("PieceCacheLimit", intSetting);
  settings->setValue("PieceCacheLimit", intSetting);

  intSetting = this->Internal->Height->text().toInt();
  if (intSetting < 1)
    {
    intSetting = 1;
    }
  QUICKSETVAL("Height", intSetting);
  settings->setValue("Height", intSetting);

  intSetting = this->Internal->Degree->text().toInt();
  if (intSetting < 2)
    {
    intSetting = 2;
    }
  QUICKSETVAL("Degree", intSetting);
  settings->setValue("Degree", intSetting);

  intSetting = this->Internal->Rate->text().toInt();
  if (intSetting < 1)
    {
    intSetting = 1;
    }
  QUICKSETVAL("Rate", intSetting);
  settings->setValue("Rate", intSetting);

  intSetting = this->Internal->MaxSplits->text().toInt();
  if (intSetting < -1)
    {
    intSetting = -1;
    }
  QUICKSETVAL("MaxSplits", intSetting);
  settings->setValue("MaxSplits", intSetting);

  helper->UpdateVTKObjects();
  settings->endGroup();
  settings->alertSettingsModified();
}

//-----------------------------------------------------------------------------
void pqGlobalAdaptiveViewOptions::resetChanges()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();

  settings->beginGroup("adaptiveView");

  QVariant val = settings->value("EnableStreamMessages", false);
  this->Internal->EnableStreamMessages->setChecked(val.toBool());

  val = settings->value("UsePrioritization", true);
  this->Internal->UsePrioritization->setChecked(val.toBool());

  val = settings->value("UseViewOrdering", true);
  this->Internal->UseViewOrdering->setChecked(val.toBool());

  val = settings->value("PieceCacheLimit", 16);
  this->Internal->PieceCacheLimit->setText(val.toString());

  val = settings->value("ShowOn", 1);
  this->Internal->ShowOn->setCurrentIndex(1);

  val = settings->value("Height", 4);
  this->Internal->Height->setText(val.toString());

  val = settings->value("Degree", 2);
  this->Internal->Degree->setText(val.toString());

  val = settings->value("Rate", 8);
  this->Internal->Rate->setText(val.toString());

  val = settings->value("MaxSplits", -1);
  this->Internal->MaxSplits->setText(val.toString());

  settings->endGroup();
}

