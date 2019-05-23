/*=========================================================================

   Program: ParaView
   Module:  pqExportInspector.cxx

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
#include "pqExportInspector.h"
#include "ui_pqExportInspector.h"

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
#include "pqServerManagerModel.h"
#include "pqView.h"

#include "vtkSMExportProxyDepot.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSMWriterFactory.h"
#include "vtkStringList.h"

#include <sstream>

//=============================================================================
class pqExportInspector::pqInternals
{
public:
  Ui::ExportInspector Ui;
  pqExportInspector* self;

  pqProxyWidget* GlobalOptionsUI;

  pqInternals(pqExportInspector* inSelf)
    : self(inSelf)
  {
    this->Ui.setupUi(inSelf);
    this->GlobalOptionsUI = nullptr;
  }

  ~pqInternals() { delete this->GlobalOptionsUI; }
};

//-----------------------------------------------------------------------------
pqExportInspector::pqExportInspector(
  QWidget* parentObject, Qt::WindowFlags f, bool /* arg_autotracking */)
  : Superclass(parentObject, f)
  , Internals(new pqExportInspector::pqInternals(this))
{
  // default to non-advanced
  this->Internals->Ui.advanced->setChecked(false);

  pqActiveObjects& ao = pqActiveObjects::instance();
  QObject::connect(&ao, SIGNAL(sourceChanged(pqPipelineSource*)), this, SLOT(Update()));
  QObject::connect(&ao, SIGNAL(viewChanged(pqView*)), this, SLOT(Update()));

  QObject::connect(
    this->Internals->Ui.filterChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  QObject::connect(this->Internals->Ui.filterChoice, SIGNAL(currentIndexChanged(int)), this,
    SLOT(UpdateWriterCheckbox()));
  QObject::connect(this->Internals->Ui.filterFormat, SIGNAL(currentIndexChanged(int)), this,
    SLOT(UpdateWriterCheckbox()));
  QObject::connect(this->Internals->Ui.filterFormat, SIGNAL(highlighted(int)), this,
    SLOT(UpdateWriterCheckbox(int)));
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
  QObject::connect(this->Internals->Ui.viewFormat, SIGNAL(highlighted(int)), this,
    SLOT(UpdateScreenshotCheckbox(int)));
  QObject::connect(
    this->Internals->Ui.viewExtract, SIGNAL(toggled(bool)), this, SLOT(ExportView(bool)));
  QObject::connect(
    this->Internals->Ui.viewConfigure, SIGNAL(clicked()), this, SLOT(ConfigureScreenshotProxy()));
  QObject::connect(this->Internals->Ui.viewExtract, SIGNAL(toggled(bool)),
    this->Internals->Ui.viewConfigure, SLOT(setEnabled(bool)));

  QObject::connect(this->Internals->Ui.advanced, SIGNAL(toggled(bool)), this, SLOT(Advanced(bool)));

  QObject::connect(this->Internals->Ui.Help, SIGNAL(pressed()), this, SLOT(Help()));

  QObject::connect(this->Internals->Ui.searchBox, SIGNAL(textChanged(const QString&)), this,
    SLOT(Search(const QString&)));
}

//-----------------------------------------------------------------------------
pqExportInspector::~pqExportInspector()
{
}

//-----------------------------------------------------------------------------
void pqExportInspector::showEvent(QShowEvent* e)
{
  (void)e;
  this->Update();
}

//-----------------------------------------------------------------------------
void pqExportInspector::Update()
{
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqActiveObjects& ao = pqActiveObjects::instance();

  // filters and writers
  // turn off handling temporarily while we set up the menu
  QObject::disconnect(
    this->Internals->Ui.filterChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  // the filters we might configure for export
  QList<pqPipelineSource*> filters = smModel->findItems<pqPipelineSource*>();
  QComboBox* filterChoice = this->Internals->Ui.filterChoice;

  QString current = filterChoice->currentText();
  pqPipelineSource* activeFilter = ao.activeSource();
  if (activeFilter)
  {
    current = activeFilter->getSMName();
  }
  filterChoice->clear();
  foreach (auto i, filters)
  {
    filterChoice->addItem(i->getSMName());
    if (i->getSMName() == current)
    {
      filterChoice->setCurrentText(current);
    }
    // reflect name changes, as happens when we open a Catalyst produced dataset with channel info
    QObject::connect(i, SIGNAL(nameChanged(pqServerManagerModelItem*)), this, SLOT(Update()),
      Qt::UniqueConnection);
  }
  // turn handling back on
  QObject::connect(
    this->Internals->Ui.filterChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  this->PopulateWriterFormats();

  // views and writers
  // turn off handling temporarily while we set up the menu
  QObject::disconnect(
    this->Internals->Ui.viewChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  // the views we might configure for export
  QList<pqView*> views = smModel->findItems<pqView*>();
  QComboBox* viewChoice = this->Internals->Ui.viewChoice;
  current = viewChoice->currentText();

  pqView* activeView = static_cast<pqView*>(ao.activeView());
  if (activeView)
  {
    current = activeView->getSMName();
  }

  viewChoice->clear();
  foreach (auto i, views)
  {
    viewChoice->addItem(i->getSMName());
    if (i->getSMName() == current)
    {
      viewChoice->setCurrentText(current);
    }
    // reflect name changes
    QObject::connect(i, SIGNAL(nameChanged(pqServerManagerModelItem*)), this, SLOT(Update()),
      Qt::UniqueConnection);
  }
  // turn handling back on
  QObject::connect(
    this->Internals->Ui.viewChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(Update()));
  this->PopulateViewFormats();

  // the global options
  this->UpdateGlobalOptions();
}

//-----------------------------------------------------------------------------
void pqExportInspector::UpdateGlobalOptions()
{
  this->UpdateGlobalOptions(QString(""));
}

//-----------------------------------------------------------------------------
void pqExportInspector::UpdateGlobalOptions(const QString& searchString)
{
  vtkSMExportProxyDepot* ed =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetExportDepot();
  vtkSMProxy* globalProxy = ed->GetGlobalOptions();

  if (this->Internals->GlobalOptionsUI)
  {
    delete this->Internals->GlobalOptionsUI;
  }

  this->Internals->GlobalOptionsUI = new pqProxyWidget(globalProxy, this->Internals->Ui.container);
  this->Internals->GlobalOptionsUI->filterWidgets(
    this->Internals->Ui.advanced->isChecked(), searchString);
  this->Internals->GlobalOptionsUI->setApplyChangesImmediately(true);
  this->Internals->GlobalOptionsUI->show();
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
void pqExportInspector::PopulateWriterFormats()
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
  auto sl = vtkSmartPointer<vtkStringList>::New();
  wf->GetGroups(sl);
  // we want our particular approved writers
  for (int i = 0; i < sl->GetNumberOfStrings(); i++)
  {
    wf->RemoveGroup(sl->GetString(i));
  }
  wf->AddGroup("insitu2_writer_parameters");

  wf->UpdateAvailableWriters();

  vtkSMSourceProxy* filter;
  int portnum;
  ::getFilterProxyAndPort(filterName, filter, portnum);
  if (!filter)
  {
    return;
  }
  std::string availproxies = wf->GetSupportedWriterProxies(filter, portnum);
  std::stringstream ss(availproxies);
  std::string item;
  std::string firstchecked = "none";
  while (std::getline(ss, item, ';'))
  {
    if (firstchecked == "none" && this->IsWriterChecked(filterName, QString::fromStdString(item)))
    {
      firstchecked = item;
    }
    if (item != "Cinema image options" || this->Internals->Ui.advanced->isChecked())
    {
      this->Internals->Ui.filterFormat->addItem(item.c_str());
    }
  }
  if (firstchecked != "none")
  {
    this->Internals->Ui.filterFormat->setCurrentText(firstchecked.c_str());
  }
}

//-----------------------------------------------------------------------------
void pqExportInspector::ExportFilter(bool enableWriter)
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

    // signify that this is now on
    writerProxy->SetAnnotation("enabled", "1");

    // use a decent default for the eventually exported filename
    std::string filename = vtkSMPropertyHelper(writerProxy, "CatalystFilePattern").GetAsString();
    if (filename.find("filename") != std::string::npos)
    {
      filename.replace(0, 8, filterName.toStdString());
      vtkSMPropertyHelper(writerProxy, "CatalystFilePattern").Set(filename.c_str());
    }

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
void pqExportInspector::ConfigureWriterProxy()
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
void pqExportInspector::UpdateWriterCheckbox(int i)
{
  if (i != -1)
  {
    // dynamically highlighting, don't affect any state, just change the widget
    QObject::disconnect(
      this->Internals->Ui.filterExtract, SIGNAL(toggled(bool)), this, SLOT(ExportFilter(bool)));
  }
  this->InternalWriterCheckbox(i);
  if (i != -1)
  {
    QObject::connect(
      this->Internals->Ui.filterExtract, SIGNAL(toggled(bool)), this, SLOT(ExportFilter(bool)));
  }
}

//-----------------------------------------------------------------------------
bool pqExportInspector::IsWriterChecked(const QString& filterName, const QString& writerName)
{
  if (filterName == "" || writerName == "")
  {
    return false;
  }

  vtkSMSourceProxy* filter;
  int portnum;
  ::getFilterProxyAndPort(filterName, filter, portnum);
  if (!filter)
  {
    return false;
  }

  vtkSMExportProxyDepot* ed =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetExportDepot();
  if (!ed->HasWriterProxy(filterName.toStdString().c_str(), writerName.toStdString().c_str()))
  {
    return false;
  }

  vtkSMSourceProxy* writerProxy =
    ed->GetWriterProxy(filter, filterName.toStdString().c_str(), writerName.toStdString().c_str());
  const char* enablestate = writerProxy->GetAnnotation("enabled");
  if (enablestate)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
void pqExportInspector::InternalWriterCheckbox(int i)
{
  QString filterName = this->Internals->Ui.filterChoice->currentText();
  QString writerName = this->Internals->Ui.filterFormat->currentText();
  if (i != -1)
  {
    writerName = this->Internals->Ui.filterFormat->itemText(i);
  }
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
void pqExportInspector::PopulateViewFormats()
{
  QComboBox* viewChoice = this->Internals->Ui.viewChoice;
  QString current = viewChoice->currentText();
  QComboBox* viewFormat = this->Internals->Ui.viewFormat;
  viewFormat->clear();
  // todo: customize this based on type of view
  std::vector<QString> writers;
  writers.push_back(QString("PNG image (*.png)"));
  writers.push_back(QString("JPG image (*.jpg)"));
  writers.push_back(QString("TIFF image (*.tif)"));
  writers.push_back(QString("BMP image (*.bmp)"));
  writers.push_back(QString("Cinema image database (*.cdb)"));
  for (auto i = writers.begin(); i != writers.end(); i++)
  {
    viewFormat->addItem(*i);
    if (this->IsScreenShotChecked(current, *i))
    {
      viewFormat->setCurrentText(*i);
    }
  }
}

//-----------------------------------------------------------------------------
void pqExportInspector::ExportView(bool enableSS)
{
  QString viewName = this->Internals->Ui.viewChoice->currentText();
  QString ssName = this->Internals->Ui.viewFormat->currentText();
  if (viewName == "" || ssName == "")
  {
    return;
  }

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqView* pqview = smModel->findItem<pqView*>(viewName);
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

    // signify that this is now on
    ssProxy->SetAnnotation("enabled", "1");

    // use a decent default for the eventually exported filename
    std::string filename = vtkSMPropertyHelper(ssProxy, "CatalystFilePattern").GetAsString();
    if (filename.find("filename") == 0)
    {
      filename.replace(0, 8, viewName.toStdString());
      vtkSMPropertyHelper(ssProxy, "CatalystFilePattern").Set(filename.c_str());
    }

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
void pqExportInspector::ConfigureScreenshotProxy()
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
  pqView* pqview = smModel->findItem<pqView*>(viewName);
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
void pqExportInspector::UpdateScreenshotCheckbox(int i)
{
  if (i != -1)
  {
    // dynamically highlighting, don't affect any state, just change the widget
    QObject::disconnect(
      this->Internals->Ui.viewExtract, SIGNAL(toggled(bool)), this, SLOT(ExportView(bool)));
  }
  this->InternalScreenshotCheckbox(i);
  if (i != -1)
  {
    QObject::connect(
      this->Internals->Ui.viewExtract, SIGNAL(toggled(bool)), this, SLOT(ExportView(bool)));
  }
}

//-----------------------------------------------------------------------------
bool pqExportInspector::IsScreenShotChecked(const QString& viewName, const QString& writerName)
{
  if (viewName == "" || writerName == "")
  {
    return false;
  }

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqView* pqview = smModel->findItem<pqView*>(viewName);
  vtkSMViewProxy* view = pqview->getViewProxy();
  if (!view)
  {
    return false;
  }

  vtkSMExportProxyDepot* ed =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetExportDepot();
  if (!ed->HasScreenshotProxy(viewName.toStdString().c_str(), writerName.toStdString().c_str()))
  {
    return false;
  }

  vtkSMSaveScreenshotProxy* writerProxy = vtkSMSaveScreenshotProxy::SafeDownCast(
    ed->GetScreenshotProxy(view, viewName.toStdString().c_str(), writerName.toStdString().c_str()));
  const char* enablestate = writerProxy->GetAnnotation("enabled");
  if (enablestate)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
void pqExportInspector::InternalScreenshotCheckbox(int i)
{
  QString viewName = this->Internals->Ui.viewChoice->currentText();
  QString writerName = this->Internals->Ui.viewFormat->currentText();
  if (i != -1)
  {
    writerName = this->Internals->Ui.viewFormat->itemText(i);
  }
  if (viewName == "" || writerName == "")
  {
    this->Internals->Ui.viewExtract->setChecked(false);
    return;
  }

  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqView* pqview = smModel->findItem<pqView*>(viewName);
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
void pqExportInspector::Advanced(bool vtkNotUsed(setting))
{
  this->UpdateGlobalOptions();
  this->PopulateWriterFormats();
}

//-----------------------------------------------------------------------------
void pqExportInspector::Help()
{
#ifdef PARAVIEW_USE_QTHELP
  // this is better than nothing, but we want a custom page
  pqHelpReaction::showProxyHelp("coprocessing", "CatalystGlobalOptions");
#endif
}

//-----------------------------------------------------------------------------
void pqExportInspector::Search(const QString& searchString)
{
  this->UpdateGlobalOptions(searchString);
}
