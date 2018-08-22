/*=========================================================================

   Program: ParaView
   Module:  pqCatalystExportInspector.cxx

   Copyright (c) 2018 Kitware Inc.
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
#include "pqCatalystExportInspector.h"
#include "ui_pqCatalystExportInspector.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#ifdef PARAVIEW_USE_QTHELP
#include "pqHelpReaction.h"
#endif
#include "pqCoreUtilities.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxyWidget.h"
#include "pqProxyWidgetDialog.h"
#include "pqRenderViewBase.h"
#include "pqServerManagerModel.h"

#include "vtkSMExportProxyDepot.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSMWriterFactory.h"

#include <sstream>

//=============================================================================
class pqCatalystExportInspector::pqInternals
{
public:
  Ui::CatalystExportInspector Ui;
  pqCatalystExportInspector* self;

  pqProxyWidget* GlobalOptionsUI;

  pqInternals(pqCatalystExportInspector* inSelf)
    : self(inSelf)
  {
    this->Ui.setupUi(inSelf);
    this->GlobalOptionsUI = nullptr;
  }

  ~pqInternals() { delete this->GlobalOptionsUI; }
};

//-----------------------------------------------------------------------------
pqCatalystExportInspector::pqCatalystExportInspector(
  QWidget* parentObject, Qt::WindowFlags f, bool /* arg_autotracking */)
  : Superclass(parentObject, f)
  , Internals(new pqCatalystExportInspector::pqInternals(this))
{
  pqActiveObjects& ao = pqActiveObjects::instance();
  QObject::connect(&ao, SIGNAL(sourceChanged(pqPipelineSource*)), this, SLOT(SourceUpdated()));
  QObject::connect(&ao, SIGNAL(viewChanged(pqView*)), this, SLOT(ViewUpdated()));

  QObject::connect(
    this->Internals->Ui.filterChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  QObject::connect(this->Internals->Ui.filterChoice, SIGNAL(currentIndexChanged(int)), this,
    SLOT(UpdateWriterCheckbox()));
  QObject::connect(this->Internals->Ui.filterFormat, SIGNAL(currentIndexChanged(int)), this,
    SLOT(UpdateWriterCheckbox()));
  QObject::connect(
    this->Internals->Ui.filterExtract, SIGNAL(toggled(bool)), this, SLOT(ExportFilter(bool)));
  QObject::connect(
    this->Internals->Ui.filterConfigure, SIGNAL(clicked()), this, SLOT(ConfigureWriterProxy()));
  QObject::connect(this->Internals->Ui.filterExtract, SIGNAL(toggled(bool)),
    this->Internals->Ui.filterConfigure, SLOT(setEnabled(bool)));

  QObject::connect(
    this->Internals->Ui.viewChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  QObject::connect(this->Internals->Ui.viewChoice, SIGNAL(currentIndexChanged(int)), this,
    SLOT(UpdateScreenshotCheckbox()));
  QObject::connect(this->Internals->Ui.viewFormat, SIGNAL(currentIndexChanged(int)), this,
    SLOT(UpdateScreenshotCheckbox()));
  QObject::connect(
    this->Internals->Ui.viewExtract, SIGNAL(toggled(bool)), this, SLOT(ExportView(bool)));
  QObject::connect(
    this->Internals->Ui.viewConfigure, SIGNAL(clicked()), this, SLOT(ConfigureScreenshotProxy()));
  QObject::connect(this->Internals->Ui.viewExtract, SIGNAL(toggled(bool)),
    this->Internals->Ui.viewConfigure, SLOT(setEnabled(bool)));

  QObject::connect(this->Internals->Ui.advanced, SIGNAL(toggled(bool)), this, SLOT(Advanced(bool)));

  QObject::connect(this->Internals->Ui.Help, SIGNAL(pressed()), this, SLOT(Help()));
}

//-----------------------------------------------------------------------------
pqCatalystExportInspector::~pqCatalystExportInspector()
{
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::showEvent(QShowEvent* e)
{
  (void)e;
  this->MakeGlobalProxy();
  this->Update();
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::SourceUpdated()
{
  this->Update();
  pqActiveObjects& ao = pqActiveObjects::instance();
  auto activeSource = ao.activeSource();
  if (!activeSource)
  {
    return;
  }
  QString newname = activeSource->getSMName();
  this->Internals->Ui.filterChoice->setCurrentText(newname);
  this->Update();
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::ViewUpdated()
{
  this->Update();
  pqActiveObjects& ao = pqActiveObjects::instance();
  auto activeView = ao.activeView();
  if (!activeView)
  {
    return;
  }
  QString newname = activeView->getSMName();
  this->Internals->Ui.viewChoice->setCurrentText(newname);
  this->Update();
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::Update()
{
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();

  // filters and writers
  // turn off handling temporaily while we set up the menu
  QObject::disconnect(
    this->Internals->Ui.filterChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  // the filters we might configure for export
  QList<pqPipelineSource*> filters = smModel->findItems<pqPipelineSource*>();
  QComboBox* filterChoice = this->Internals->Ui.filterChoice;
  QString current = filterChoice->currentText();
  filterChoice->clear();
  foreach (auto i, filters)
  {
    filterChoice->addItem(i->getSMName());
    if (i->getSMName() == current)
    {
      filterChoice->setCurrentText(current);
    }
  }
  // turn handling back on
  QObject::connect(
    this->Internals->Ui.filterChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  this->PopulateWriterFormats();

  // views and writers
  // turn off handling temporaily while we set up the menu
  QObject::disconnect(
    this->Internals->Ui.viewChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  // the views we might configure for export
  QList<pqRenderViewBase*> views = smModel->findItems<pqRenderViewBase*>();
  QComboBox* viewChoice = this->Internals->Ui.viewChoice;
  current = viewChoice->currentText();
  viewChoice->clear();
  foreach (auto i, views)
  {
    viewChoice->addItem(i->getSMName());
    if (i->getSMName() == current)
    {
      viewChoice->setCurrentText(current);
    }
  }
  // turn handling back on
  QObject::connect(
    this->Internals->Ui.viewChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  this->PopulateViewFormats();

  // the global options
  if (this->Internals->GlobalOptionsUI)
  {
    this->Internals->GlobalOptionsUI->updatePanel();
    this->Internals->GlobalOptionsUI->show();
  }
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::MakeGlobalProxy()
{
  vtkSMExportProxyDepot* ed =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetExportDepot();
  vtkSMProxy* globalProxy = ed->GetGlobalOptions();

  this->Internals->GlobalOptionsUI = new pqProxyWidget(globalProxy, this->Internals->Ui.container);
  // this->Internals->GlobalOptionsUI->filterWidgets(this->Internals->Ui.advanced->isChecked());
  // //why won't update UI correctly?
  this->Internals->GlobalOptionsUI->filterWidgets(true);
  this->Internals->GlobalOptionsUI->setApplyChangesImmediately(true);
}

//-----------------------------------------------------------------------------
namespace
{
void getFilterProxyAndPort(QString filterName, vtkSMSourceProxy*& filter, int& portnum)
{
  filter = nullptr;
  portnum = 0;

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineSource* proxy = smModel->findItem<pqPipelineSource*>(filterName);
  if (!proxy)
  {
    return;
  }
  pqOutputPort* port = proxy->getOutputPort(0);
  filter = vtkSMSourceProxy::SafeDownCast(port->getSource()->getProxy());
  portnum = port->getPortNumber();
  return;
}
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::PopulateWriterFormats()
{
  QComboBox* writerChoice = this->Internals->Ui.filterFormat;
  writerChoice->clear();

  QString filterName = this->Internals->Ui.filterChoice->currentText();
  if (filterName == "")
  {
    return;
  }

  // discover the list of all possible writers for the current filter
  vtkSmartPointer<vtkSMWriterFactory> wf = vtkSmartPointer<vtkSMWriterFactory>::New();
  wf->UpdateAvailableWriters();
  vtkSMSourceProxy* filter;
  int portnum;
  ::getFilterProxyAndPort(filterName, filter, portnum);
  if (!filter)
  {
    return;
  }
  std::string availproxies = wf->GetSupportedWriterProxies(filter, portnum, "CatalystApproved");
  std::stringstream ss(availproxies);
  std::string item;
  while (std::getline(ss, item, ';'))
  {
    this->Internals->Ui.filterFormat->addItem(item.c_str());
  }
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::ExportFilter(bool enableWriter)
{
  QString filterName = this->Internals->Ui.filterChoice->currentText();
  QString writerName = this->Internals->Ui.filterFormat->currentText();
  if (filterName == "" || writerName == "")
  {
    return;
  }

  vtkSMSourceProxy* filter;
  int portnum;
  ::getFilterProxyAndPort(filterName, filter, portnum);
  if (!filter)
  {
    return;
  }

  vtkSMExportProxyDepot* ed =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetExportDepot();
  if (enableWriter)
  {
    vtkSMSourceProxy* writerProxy = ed->GetWriterProxy(
      filter, filterName.toStdString().c_str(), writerName.toStdString().c_str());

    writerProxy->SetAnnotation("enabled", "1");

    // other than the cinema writer, we only allow one at a time
    if (writerName != "Cinema image options")
    {
      for (int i = 0; i < this->Internals->Ui.filterFormat->count(); i++)
      {
        auto atit = this->Internals->Ui.filterFormat->itemText(i);
        if (atit == writerName || (atit == "Cinema image options"))
        {
          continue;
        }
        if (ed->HasWriterProxy(filterName.toStdString().c_str(), atit.toStdString().c_str()))
        {
          vtkSMSourceProxy* otherWriter = ed->GetWriterProxy(
            filter, filterName.toStdString().c_str(), atit.toStdString().c_str());
          otherWriter->SetAnnotation("enabled", nullptr);
        }
      }
    }
  }
  else
  {
    if (ed->HasWriterProxy(filterName.toStdString().c_str(), writerName.toStdString().c_str()))
    {
      vtkSMSourceProxy* writerProxy = ed->GetWriterProxy(
        filter, filterName.toStdString().c_str(), writerName.toStdString().c_str());
      writerProxy->SetAnnotation("enabled", nullptr);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::ConfigureWriterProxy()
{
  if (!this->Internals->Ui.filterExtract->isChecked())
  {
    return;
  }

  QString filterName = this->Internals->Ui.filterChoice->currentText();
  QString writerName = this->Internals->Ui.filterFormat->currentText();
  if (filterName == "" || writerName == "")
  {
    return;
  }

  vtkSMSourceProxy* filter;
  int portnum;
  ::getFilterProxyAndPort(filterName, filter, portnum);
  if (!filter)
  {
    return;
  }

  vtkSMExportProxyDepot* ed =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetExportDepot();
  vtkSMSourceProxy* writerProxy =
    ed->GetWriterProxy(filter, filterName.toStdString().c_str(), writerName.toStdString().c_str());

  pqProxyWidgetDialog dialog(writerProxy, pqCoreUtilities::mainWidget());
  dialog.setObjectName("SaveDataDialog");
  dialog.setApplyChangesImmediately(true);
  dialog.setWindowTitle("Save Data Options");
  dialog.setEnableSearchBar(true);
  dialog.setSettingsKey("SaveDataDialog");
  dialog.exec();
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::UpdateWriterCheckbox()
{
  QString filterName = this->Internals->Ui.filterChoice->currentText();
  QString writerName = this->Internals->Ui.filterFormat->currentText();
  if (filterName == "" || writerName == "")
  {
    this->Internals->Ui.filterExtract->setChecked(false);
    return;
  }

  vtkSMSourceProxy* filter;
  int portnum;
  ::getFilterProxyAndPort(filterName, filter, portnum);
  if (!filter)
  {
    return;
  }

  vtkSMExportProxyDepot* ed =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetExportDepot();
  if (!ed->HasWriterProxy(filterName.toStdString().c_str(), writerName.toStdString().c_str()))
  {
    this->Internals->Ui.filterExtract->setChecked(false);
    return;
  }

  vtkSMSourceProxy* writerProxy =
    ed->GetWriterProxy(filter, filterName.toStdString().c_str(), writerName.toStdString().c_str());
  const char* enablestate = writerProxy->GetAnnotation("enabled");
  if (enablestate)
  {
    this->Internals->Ui.filterExtract->setChecked(true);
  }
  else
  {
    this->Internals->Ui.filterExtract->setChecked(false);
  }
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::PopulateViewFormats()
{
  QComboBox* viewChoice = this->Internals->Ui.viewChoice;
  QString current = viewChoice->currentText();
  QComboBox* viewFormat = this->Internals->Ui.viewFormat;
  viewFormat->clear();
  // todo: customize this based on type of view
  viewFormat->addItem("PNG image (*.png)");
  viewFormat->addItem("JPG image (*.jpg)");
  viewFormat->addItem("TIFF image (*.tif)");
  viewFormat->addItem("BMP image (*.bmp)");
  viewFormat->addItem("PPM image (*.ppm)");
  viewFormat->addItem("Cinema image database (*.cdb)");
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::ExportView(bool enableSS)
{
  QString viewName = this->Internals->Ui.viewChoice->currentText();
  QString ssName = this->Internals->Ui.viewFormat->currentText();
  if (viewName == "" || ssName == "")
  {
    return;
  }

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqRenderViewBase* pqview = smModel->findItem<pqRenderViewBase*>(viewName);
  vtkSMViewProxy* view = pqview->getViewProxy();
  if (!view)
  {
    return;
  }

  vtkSMExportProxyDepot* ed =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetExportDepot();
  if (enableSS)
  {
    vtkSMSaveScreenshotProxy* ssProxy = vtkSMSaveScreenshotProxy::SafeDownCast(
      ed->GetScreenshotProxy(view, viewName.toStdString().c_str(), ssName.toStdString().c_str()));

    // create the corresponding writer subproxy
    // Note: I would do this in ExportProxyDepot except no rendering dependency there
    std::string formatS = ssName.toStdString();
    size_t dotP = formatS.find_first_of(".") + 1;
    size_t rparenP = formatS.find_last_of(")");
    std::string extension = "dontcare." + formatS.substr(dotP, rparenP - dotP);
    ssProxy->UpdateDefaultsAndVisibilities(extension.c_str());
    ssProxy->SetAnnotation("enabled", "1");

    // including the cinema writer, we only allow one at a time
    for (int i = 0; i < this->Internals->Ui.viewFormat->count(); i++)
    {
      auto atit = this->Internals->Ui.viewFormat->itemText(i);
      if (atit == ssName)
      {
        continue;
      }
      if (ed->HasScreenshotProxy(viewName.toStdString().c_str(), atit.toStdString().c_str()))
      {
        vtkSMSaveScreenshotProxy* otherSS = vtkSMSaveScreenshotProxy::SafeDownCast(
          ed->GetScreenshotProxy(view, viewName.toStdString().c_str(), atit.toStdString().c_str()));

        otherSS->SetAnnotation("enabled", nullptr);
      }
    }
  }
  else
  {
    if (ed->HasScreenshotProxy(viewName.toStdString().c_str(), ssName.toStdString().c_str()))
    {
      vtkSMSaveScreenshotProxy* ssProxy = vtkSMSaveScreenshotProxy::SafeDownCast(
        ed->GetScreenshotProxy(view, viewName.toStdString().c_str(), ssName.toStdString().c_str()));

      ssProxy->SetAnnotation("enabled", nullptr);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::ConfigureScreenshotProxy()
{
  if (!this->Internals->Ui.viewExtract->isChecked())
  {
    return;
  }

  QString viewName = this->Internals->Ui.viewChoice->currentText();
  QString ssName = this->Internals->Ui.viewFormat->currentText();
  if (viewName == "" || ssName == "")
  {
    return;
  }

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqRenderViewBase* pqview = smModel->findItem<pqRenderViewBase*>(viewName);
  vtkSMViewProxy* view = pqview->getViewProxy();
  if (!view)
  {
    return;
  }

  vtkSMExportProxyDepot* ed =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetExportDepot();

  vtkSMSaveScreenshotProxy* ssProxy = vtkSMSaveScreenshotProxy::SafeDownCast(
    ed->GetScreenshotProxy(view, viewName.toStdString().c_str(), ssName.toStdString().c_str()));

  pqProxyWidgetDialog dialog(ssProxy, pqCoreUtilities::mainWidget());
  dialog.setObjectName("SaveScreenshotDialog");
  dialog.setApplyChangesImmediately(true);
  dialog.setWindowTitle("Save Screenshot Options");
  dialog.setEnableSearchBar(true);
  dialog.setSettingsKey("SaveScreenshotDialog");
  dialog.exec();
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::UpdateScreenshotCheckbox()
{
  QString viewName = this->Internals->Ui.viewChoice->currentText();
  QString writerName = this->Internals->Ui.viewFormat->currentText();
  if (viewName == "" || writerName == "")
  {
    this->Internals->Ui.viewExtract->setChecked(false);
    return;
  }

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqRenderViewBase* pqview = smModel->findItem<pqRenderViewBase*>(viewName);
  vtkSMViewProxy* view = pqview->getViewProxy();
  if (!view)
  {
    return;
  }

  vtkSMExportProxyDepot* ed =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetExportDepot();
  if (!ed->HasScreenshotProxy(viewName.toStdString().c_str(), writerName.toStdString().c_str()))
  {
    this->Internals->Ui.viewExtract->setChecked(false);
    return;
  }

  vtkSMSaveScreenshotProxy* writerProxy = vtkSMSaveScreenshotProxy::SafeDownCast(
    ed->GetScreenshotProxy(view, viewName.toStdString().c_str(), writerName.toStdString().c_str()));
  const char* enablestate = writerProxy->GetAnnotation("enabled");
  if (enablestate)
  {
    this->Internals->Ui.viewExtract->setChecked(true);
  }
  else
  {
    this->Internals->Ui.viewExtract->setChecked(false);
  }
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::Advanced(bool setting)
{
  if (setting)
  {
    this->Internals->Ui.filterConfigure->show();
    this->Internals->Ui.viewConfigure->show();
  }
  else
  {
    this->Internals->Ui.filterConfigure->hide();
    this->Internals->Ui.viewConfigure->hide();
  }

  if (this->Internals->GlobalOptionsUI)
  {
    delete this->Internals->GlobalOptionsUI;
    this->MakeGlobalProxy();
  }
}

//-----------------------------------------------------------------------------
void pqCatalystExportInspector::Help()
{
#ifdef PARAVIEW_USE_QTHELP
  // this is better than nothing, but we want a custom page
  pqHelpReaction::showProxyHelp("coprocessing", "CatalystGlobalOptions");
#endif
}
