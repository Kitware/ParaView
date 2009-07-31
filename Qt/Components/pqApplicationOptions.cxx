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

#include "vtkSMProxyManager.h"
#include "vtkSMProxyDefinitionIterator.h"
#include "vtkSMPropertyHelper.h"

#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqChartRepresentation.h"
#include "pqObjectInspectorWidget.h"
#include "pqPluginManager.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqSetName.h"
#include "pqSettings.h"
#include "pqViewModuleInterface.h"

#include <QMenu>
#include <QDoubleValidator>
#include <QtDebug>

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

  QObject::connect(this->Internal->ForegroundColor,
                  SIGNAL(chosenColorChanged(const QColor&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->SurfaceColor,
                  SIGNAL(chosenColorChanged(const QColor&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->BackgroundColor,
                  SIGNAL(chosenColorChanged(const QColor&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->TextAnnotationColor,
                  SIGNAL(chosenColorChanged(const QColor&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->EdgeColor,
                  SIGNAL(chosenColorChanged(const QColor&)),
                  this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->SelectionColor,
                  SIGNAL(chosenColorChanged(const QColor&)),
                  this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->ResetColorsToDefault, 
    SIGNAL(clicked()),
    this, SLOT(resetColorsToDefault()));

  QObject::connect(this->Internal->AnimationCacheGeometry,
                   SIGNAL(toggled(bool)),
                   this, SIGNAL(changesAvailable()));
  QObject::connect(this->Internal->AnimationCacheLimit,
                   SIGNAL(valueChanged(int)),
                   this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->AnimationCacheGeometry,
                   SIGNAL(toggled(bool)),
                   this->Internal->AnimationCacheLimit,
                   SLOT(setEnabled(bool)));

  QObject::connect(this->Internal->AnimationCacheGeometry,
                   SIGNAL(toggled(bool)),
                   this->Internal->AnimationCacheLimitLabel,
                   SLOT(setEnabled(bool)));

  QObject::connect(this->Internal->ChartHiddenSeries,
                   SIGNAL(itemChanged(QListWidgetItem*)),
                   this, SIGNAL(changesAvailable()));

  QObject::connect(this->Internal->ChartNewHiddenSeries, SIGNAL(clicked(bool)),
                   this, SLOT(onChartNewHiddenSeries()));
  QObject::connect(this->Internal->ChartDeleteHiddenSeries,
                   SIGNAL(clicked(bool)),
                   this, SLOT(onChartDeleteHiddenSeries()));
  QObject::connect(this->Internal->ChartResetHiddenSeries,
                   SIGNAL(clicked(bool)),
                   this, SLOT(onChartResetHiddenSeries()));

  QMenu* paletteMenu = new QMenu(this->Internal->Palette)
    << pqSetName("paletteMenu");
  this->Internal->Palette->setMenu(paletteMenu);

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxyDefinitionIterator* iter = vtkSMProxyDefinitionIterator::New();
  iter->SetModeToOneGroup();
  for (iter->Begin("palettes"); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", iter->GetKey());
    if (prototype)
      {
      paletteMenu->addAction(prototype->GetXMLLabel())
        << pqSetName(prototype->GetXMLName());
      }
    }
  iter->Delete();

  QObject::connect(paletteMenu, SIGNAL(triggered(QAction*)),
    this, SLOT(onPalette(QAction*)));
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

  settings->setValue("GlobalProperties/ForegroundColor", 
    this->Internal->ForegroundColor->chosenColor());
  settings->setValue("GlobalProperties/SurfaceColor", 
    this->Internal->SurfaceColor->chosenColor());
  settings->setValue("GlobalProperties/BackgroundColor", 
    this->Internal->BackgroundColor->chosenColor());
  settings->setValue("GlobalProperties/TextAnnotationColor", 
    this->Internal->TextAnnotationColor->chosenColor());
  settings->setValue("GlobalProperties/SelectionColor", 
    this->Internal->SelectionColor->chosenColor());
  settings->setValue("GlobalProperties/EdgeColor", 
    this->Internal->EdgeColor->chosenColor());

  pqAnimationScene::setCacheGeometrySetting(
                           this->Internal->AnimationCacheGeometry->isChecked());
  pqAnimationScene::setCacheLimitSetting(
                                  this->Internal->AnimationCacheLimit->value());

  QStringList hidden;
  for (int i = 0; i < this->Internal->ChartHiddenSeries->count(); i++)
    {
    hidden << this->Internal->ChartHiddenSeries->item(i)->text();
    }
  pqChartRepresentation::setHiddenSeriesSetting(hidden);

  pqApplicationCore::instance()->loadGlobalPropertiesFromSettings();

  // render all views.
  pqApplicationCore::instance()->render();
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
  
  this->Internal->ForegroundColor->setChosenColor(
    settings->value("GlobalProperties/ForegroundColor",
      QColor::fromRgbF(1, 1,1)).value<QColor>());
  this->Internal->SurfaceColor->setChosenColor(
    settings->value("GlobalProperties/SurfaceColor",
      QColor::fromRgbF(1, 1,1)).value<QColor>());
  this->Internal->BackgroundColor->setChosenColor(
    settings->value("GlobalProperties/BackgroundColor",
      QColor::fromRgbF(0.32, 0.34, 0.43)).value<QColor>());
  this->Internal->TextAnnotationColor->setChosenColor(
    settings->value("GlobalProperties/TextAnnotationColor",
      QColor::fromRgbF(1, 1, 1)).value<QColor>());
  this->Internal->SelectionColor->setChosenColor(
    settings->value("GlobalProperties/SelectionColor",
      QColor::fromRgbF(1, 0, 1)).value<QColor>());
  this->Internal->EdgeColor->setChosenColor(
    settings->value("GlobalProperties/EdgeColor",
      QColor::fromRgbF(0, 0, 0.5)).value<QColor>());

  this->Internal->AnimationCacheGeometry->setChecked(
                                   pqAnimationScene::getCacheGeometrySetting());
  this->Internal->AnimationCacheLimit->setValue(
                                      pqAnimationScene::getCacheLimitSetting());

  while (this->Internal->ChartHiddenSeries->count() > 0)
    {
    delete this->Internal->ChartHiddenSeries->takeItem(0);
    }
  foreach (QString hidden, pqChartRepresentation::getHiddenSeriesSetting())
    {
    QListWidgetItem *item
      = new QListWidgetItem(hidden, this->Internal->ChartHiddenSeries);
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
    }
}

//-----------------------------------------------------------------------------
void pqApplicationOptions::resetColorsToDefault()
{
  // FIXME: Need some mechanism to centralize this crap! But this has taken too
  // long already so I am leaving this for now.
  this->Internal->ForegroundColor->setChosenColor(QColor::fromRgbF(1, 1,1));
  this->Internal->SurfaceColor->setChosenColor(QColor::fromRgbF(1, 1,1));
  this->Internal->BackgroundColor->setChosenColor(
    QColor::fromRgbF(0.32, 0.34, 0.43));
  this->Internal->TextAnnotationColor->setChosenColor(QColor::fromRgbF(1, 1, 1));
  this->Internal->SelectionColor->setChosenColor(QColor::fromRgbF(1, 0, 1));
  this->Internal->EdgeColor->setChosenColor(QColor::fromRgbF(0, 0, 0.5));
}


inline QColor getQColor(vtkSMProxy* proxy, const char* pname)
{
  vtkSMPropertyHelper helper(proxy, pname);
  return QColor::fromRgbF(
    helper.GetAsDouble(0),
    helper.GetAsDouble(1),
    helper.GetAsDouble(2));
}
//-----------------------------------------------------------------------------
void pqApplicationOptions::loadPalette(const QString& paletteName)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes",
    paletteName.toAscii().data());
  if (!prototype)
    {
    qCritical() << "No such palette " << paletteName;
    return;
    }
 
  this->Internal->ForegroundColor->setChosenColor(
    ::getQColor(prototype, "ForegroundColor"));
  this->Internal->BackgroundColor->setChosenColor(
    ::getQColor(prototype, "BackgroundColor"));
  this->Internal->SurfaceColor->setChosenColor(
    ::getQColor(prototype, "SurfaceColor"));
  this->Internal->TextAnnotationColor->setChosenColor(
    ::getQColor(prototype, "TextAnnotationColor"));
  this->Internal->EdgeColor->setChosenColor(
    ::getQColor(prototype, "EdgeColor"));
  this->Internal->SelectionColor->setChosenColor(
    ::getQColor(prototype, "SelectionColor"));
}

//-----------------------------------------------------------------------------
void pqApplicationOptions::onPalette(QAction* action)
{
  this->loadPalette(action->objectName());
}

//-----------------------------------------------------------------------------
void pqApplicationOptions::onChartNewHiddenSeries()
{
  QListWidgetItem *item = new QListWidgetItem("");
  item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
  this->Internal->ChartHiddenSeries->insertItem(0, item);
  this->Internal->ChartHiddenSeries->editItem(item);

  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqApplicationOptions::onChartDeleteHiddenSeries()
{
  foreach (QListWidgetItem *item,
           this->Internal->ChartHiddenSeries->selectedItems())
    {
    int row = this->Internal->ChartHiddenSeries->row(item);
    delete this->Internal->ChartHiddenSeries->takeItem(row);
    }

  emit this->changesAvailable();
}

//-----------------------------------------------------------------------------
void pqApplicationOptions::onChartResetHiddenSeries()
{
  while (this->Internal->ChartHiddenSeries->count() > 0)
    {
    delete this->Internal->ChartHiddenSeries->takeItem(0);
    }
  foreach (QString hidden, pqChartRepresentation::defaultHiddenSeriesSetting())
    {
    QListWidgetItem *item
      = new QListWidgetItem(hidden, this->Internal->ChartHiddenSeries);
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
    }

  emit this->changesAvailable();
}
