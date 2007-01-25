/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowser.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqPipelineBrowser.cxx
/// \date 4/20/2006

#include "pqPipelineBrowser.h"

#include "pqAddSourceDialog.h"
#include "pqApplicationCore.h"
#include "pqFilterInputDialog.h"
#include "pqFlatTreeView.h"
#include "pqPipelineBrowserStateManager.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPipelineModel.h"
#include "pqPipelineModelSelectionAdaptor.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqGenericViewModule.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqSourceHistoryModel.h"
#include "pqSourceInfoFilterModel.h"
#include "pqSourceInfoGroupMap.h"
#include "pqSourceInfoIcons.h"
#include "pqSourceInfoModel.h"
#include "pqUndoStack.h"

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QPointer>
#include <QString>
#include <QtDebug>
#include <QVBoxLayout>

#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"


class pqPipelineBrowserInternal
{
public:
  pqPipelineBrowserInternal();
  ~pqPipelineBrowserInternal() {}

  // TODO: Add support for multiple servers.
  pqSourceInfoModel *FilterModel;
  QString LastFilterGroup;
  QPointer<pqGenericViewModule> ViewModule;
};


//----------------------------------------------------------------------------
pqPipelineBrowserInternal::pqPipelineBrowserInternal()
  : LastFilterGroup()
{
  this->FilterModel = 0;
  this->ViewModule = 0;
}


//----------------------------------------------------------------------------
pqPipelineBrowser::pqPipelineBrowser(QWidget *widgetParent)
  : QWidget(widgetParent)
{
  this->Internal = new pqPipelineBrowserInternal();
  this->Model = 0;
  this->TreeView = 0;
  this->Icons = new pqSourceInfoIcons(this);
  this->FilterGroups = new pqSourceInfoGroupMap(this);
  this->FilterHistory = new pqSourceHistoryModel(this);
  this->Manager = new pqPipelineBrowserStateManager(this);

  // Set the icons for the history models.
  this->FilterHistory->setIcons(this->Icons, pqSourceInfoIcons::Filter);

  // Get the pipeline model from the pipeline data.
  this->Model = new pqPipelineModel(this);

  // Connect the model to the ServerManager model.
  pqServerManagerModel *smModel = 
      pqApplicationCore::instance()->getServerManagerModel();
  
  this->connect(smModel, SIGNAL(serverAdded(pqServer*)),
      this->Model, SLOT(addServer(pqServer*)));
  this->connect(smModel, SIGNAL(aboutToRemoveServer(pqServer *)),
      this->Model, SLOT(startRemovingServer(pqServer *)));
  this->connect(smModel, SIGNAL(serverRemoved(pqServer*)),
      this->Model, SLOT(removeServer(pqServer*)));
  this->connect(smModel, SIGNAL(sourceAdded(pqPipelineSource*)),
      this->Model, SLOT(addSource(pqPipelineSource*)));
  this->connect(smModel, SIGNAL(sourceRemoved(pqPipelineSource*)),
      this->Model, SLOT(removeSource(pqPipelineSource*)));
  this->connect(smModel,
      SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*)),
      this->Model,
      SLOT(addConnection(pqPipelineSource*, pqPipelineSource*)));
  this->connect(smModel,
      SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*)),
      this->Model,
      SLOT(removeConnection(pqPipelineSource*, pqPipelineSource*)));

  this->connect(smModel, SIGNAL(nameChanged(pqServerManagerModelItem *)),
      this->Model, SLOT(updateItemName(pqServerManagerModelItem *)));
  this->connect(smModel,
      SIGNAL(sourceDisplayChanged(pqPipelineSource *, pqConsumerDisplay*)),
      this->Model,
      SLOT(updateDisplays(pqPipelineSource *, pqConsumerDisplay*)));

  // Create a flat tree view to display the pipeline.
  this->TreeView = new pqFlatTreeView(this);
  this->TreeView->setObjectName("PipelineView");
  this->TreeView->getHeader()->hide();
  this->TreeView->setModel(this->Model);
  this->TreeView->installEventFilter(this);
  this->TreeView->getHeader()->moveSection(1, 0);
  this->TreeView->setSelectionMode(pqFlatTreeView::ExtendedSelection);

  // Use the tree view's font as the base for the model's modified
  // font.
  QFont modifiedFont = this->TreeView->font();
  modifiedFont.setBold(true);
  this->Model->setModifiedFont(modifiedFont);

  // Listen for index clicked signals to change visibility.
  this->connect(this->TreeView, SIGNAL(clicked(const QModelIndex &)),
      this, SLOT(handleIndexClicked(const QModelIndex &)));

  // Make sure the tree items get expanded when new descendents
  // are added.
  this->connect(this->Model, SIGNAL(firstChildAdded(const QModelIndex &)),
      this->TreeView, SLOT(expand(const QModelIndex &)));

  // Use the model's move and restore signals to keep track of
  // selected and expanded indexes.
  this->Manager->setModelAndView(this->Model, this->TreeView);

  // The tree view should have a context menu based on the selected
  // items. The context menu policy should be set to custom for this
  // behavior.
  this->TreeView->setContextMenuPolicy(Qt::CustomContextMenu);

  // Add the tree view to the layout.
  QVBoxLayout *boxLayout = new QVBoxLayout(this);
  boxLayout->setMargin(0);
  boxLayout->addWidget(this->TreeView);

  // Create the selection adaptor.
  new pqPipelineModelSelectionAdaptor(this->TreeView->getSelectionModel(),
      pqApplicationCore::instance()->getSelectionModel(), this);
}

pqPipelineBrowser::~pqPipelineBrowser()
{
  delete this->Internal;
}

bool pqPipelineBrowser::eventFilter(QObject *object, QEvent *e)
{
  if(object == this->TreeView && e->type() == QEvent::KeyPress)
    {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
    if(keyEvent->key() == Qt::Key_Delete ||
        keyEvent->key() == Qt::Key_Backspace)
      {
      this->deleteSelected();
      }
    }

  return QWidget::eventFilter(object, e);
}

void pqPipelineBrowser::loadFilterInfo(vtkPVXMLElement *root)
{
  this->FilterGroups->loadSourceInfo(root);

  // TEMP: Add in the list of released filters.
  this->FilterGroups->addGroup("Released");
  this->FilterGroups->addSource("Clip", "Released");
  this->FilterGroups->addSource("Cut", "Released");
  this->FilterGroups->addSource("Threshold", "Released");
}

void pqPipelineBrowser::saveState(vtkPVXMLElement *root) const
{
  this->Manager->saveState(root);
}

void pqPipelineBrowser::restoreState(vtkPVXMLElement *root)
{
  this->Manager->restoreState(root);
}

QItemSelectionModel *pqPipelineBrowser::getSelectionModel() const
{
  return this->TreeView->getSelectionModel();
}

pqGenericViewModule *pqPipelineBrowser::getViewModule() const
{
  return this->Internal->ViewModule;
}

pqConsumerDisplay *pqPipelineBrowser::createDisplay(pqPipelineSource *source, 
    bool visible)
{
  if(!this->Internal->ViewModule ||
      !this->Internal->ViewModule->canDisplaySource(source))
    {
    return 0;
    }

  pqConsumerDisplay *display = 
      pqApplicationCore::instance()->getPipelineBuilder()->createDisplay(
      source, this->Internal->ViewModule);
  display->setVisible(visible);
  return display;
}

void pqPipelineBrowser::addSource()
{
  // TODO
}

void pqPipelineBrowser::addFilter()
{
  // Get the source input from the browser's selection model.
  QModelIndexList indexes = this->getSelectionModel()->selectedIndexes();
  if(indexes.size() != 1)
    {
    // TODO: Add support for multi-input filters like append.
    return;
    }

  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(
      this->Model->getItemFor(indexes.first()));
  if(!source)
    {
    return;
    }

  // Get the filter info model for the current server.
  pqSourceInfoModel *model = this->getFilterModel();

  // Use a proxy model to display only the allowed filters.
  QStringList allowed;
  pqSourceInfoFilterModel *filter = new pqSourceInfoFilterModel(this);
  filter->setSourceModel(model);
  this->getAllowedSources(model, source->getProxy(), allowed);
  filter->setAllowedNames(allowed);

  pqSourceInfoFilterModel *history = new pqSourceInfoFilterModel(this);
  history->setSourceModel(this->FilterHistory);
  history->setAllowedNames(allowed);

  // Set up the add filter dialog.
  pqAddSourceDialog dialog(QApplication::activeWindow());
  dialog.setSourceMap(this->FilterGroups);
  dialog.setSourceList(filter);
  dialog.setHistoryList(history);
  dialog.setSourceLabel("Filter");
  dialog.setWindowTitle("Add Filter");

  // Start the user in the previous group path.
  dialog.setPath(this->Internal->LastFilterGroup);
  if(dialog.exec() == QDialog::Accepted)
    {
    // If the user selects a filter, save the starting path and add
    // the selected filter to the history.
    dialog.getPath(this->Internal->LastFilterGroup);
    QString filterName;
    dialog.getSource(filterName);
    this->FilterHistory->addRecentSource(filterName);

    // Create the filter.
    if(!pqApplicationCore::instance()->createFilterForSource(filterName,
        source))
      {
      qCritical() << "Filter could not be created.";
      }
    }

  delete filter;
  delete history;
}

void pqPipelineBrowser::changeInput()
{
  // The change input dialog only supports one filter at a time.
  if(this->getSelectionModel()->selectedIndexes().size() != 1)
    {
    return;
    }

  QModelIndex current = this->getSelectionModel()->currentIndex();
  pqPipelineFilter *filter = dynamic_cast<pqPipelineFilter *>(
      this->Model->getItemFor(current));
  if(filter)
    {
    pqFilterInputDialog dialog(QApplication::activeWindow());
    dialog.setObjectName("ChangeInputDialog");
    pqServerManagerModel *smModel =
        pqApplicationCore::instance()->getServerManagerModel();
    pqPipelineModel *model = new pqPipelineModel(*smModel);
    dialog.setModelAndFilter(model, filter);
    if(QDialog::Accepted == dialog.exec())
      {
      // TODO: Change the inputs for all input ports.
      QStringList toAdd, toRemove;
      dialog.getFilterInputs("Input", toAdd);
      dialog.getCurrentFilterInputs("Input", toRemove);

      // Remove the items that are in both lists.
      QStringList::Iterator iter = toAdd.begin();
      while(iter != toAdd.end())
        {
        if(toRemove.contains(*iter))
          {
          toRemove.removeAll(*iter);
          iter = toAdd.erase(iter);
          }
        else
          {
          ++iter;
          }
        }

      // Remove the old connections.
      pqPipelineSource *source = 0;
      pqPipelineBuilder *builder =
          pqApplicationCore::instance()->getPipelineBuilder();
      pqUndoStack *undo = pqApplicationCore::instance()->getUndoStack();
      undo->BeginUndoSet(QString("Change Input"));
      for(iter = toRemove.begin(); iter != toRemove.end(); ++iter)
        {
        source = smModel->getPQSource(*iter);
        builder->removeConnection(source, filter);
        }

      // Add the new connections.
      for(iter = toAdd.begin(); iter != toAdd.end(); ++iter)
        {
        source = smModel->getPQSource(*iter);
        builder->addConnection(source, filter);
        }

      undo->EndUndoSet();
      }

    delete model;
    }
}

void pqPipelineBrowser::deleteSelected()
{
  QModelIndexList indexes = this->getSelectionModel()->selectedIndexes();
  if(indexes.size() != 1)
    {
    // TODO: Add support for deleting multiple items at once.
    return;
    }

  // Get the selected item(s) from the selection model.
  pqServerManagerModelItem *item = this->Model->getItemFor(indexes.first());
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
  pqServer *server = dynamic_cast<pqServer *>(item);
  if(source)
    {
    if(source->getNumberOfConsumers() == 0)
      {
      pqApplicationCore::instance()->removeSource(source);
      }
    }
  else if(server)
    {
    pqApplicationCore::instance()->removeServer(server);
    }
}

void pqPipelineBrowser::setViewModule(pqGenericViewModule *rm)
{
  this->Internal->ViewModule = rm;
  this->Model->setViewModule(rm);
}

void pqPipelineBrowser::handleIndexClicked(const QModelIndex &index)
{
  // See if the index is associated with a source.
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(
      this->Model->getItemFor(index));
  if(source)
    {
    if(index.column() == 1)
      {
      // If the column clicked is 1, the user clicked the visible icon.
      // Get the display object for the current window.
      pqConsumerDisplay *display = source->getDisplay(
          this->Internal->ViewModule);

      // If the display exists, toggle the display. Otherwise, create a
      // display for the source in the current window.
      if(!display)
        {
        display = this->createDisplay(source, true);
        }
      else
        {
        display->setVisible(!display->isVisible());
        }

      if(display)
        {
        display->renderAllViews(false);
        }
      }
    // TODO
    //else if(index.column() == 2)
    //  {
    //  // If the column clicked is 2, the user clicked the input menu.
    //  }
    }
}

pqSourceInfoModel *pqPipelineBrowser::getFilterModel()
{
  // TODO: Add support for multiple servers.
  if(!this->Internal->FilterModel)
    {
    // Get the list of available filters from the server manager.
    QStringList filters;
    vtkSMProxyManager *manager = vtkSMProxyManager::GetProxyManager();
    manager->InstantiateGroupPrototypes("filters");
    unsigned int total = manager->GetNumberOfProxies("filters_prototypes");
    for(unsigned int i = 0; i < total; i++)
      {
      filters.append(manager->GetProxyName("filters_prototypes", i));
      }

    // Create a new model for the filter groups.
    this->Internal->FilterModel = new pqSourceInfoModel(filters, this);

    // Initialize the new model.
    this->setupConnections(this->Internal->FilterModel, this->FilterGroups);
    this->Internal->FilterModel->setIcons(this->Icons,
        pqSourceInfoIcons::Filter);
    }

  return this->Internal->FilterModel;
}

void pqPipelineBrowser::setupConnections(pqSourceInfoModel *model,
    pqSourceInfoGroupMap *map)
{
  // Connect the new model to the group map and add the initial
  // items to the model.
  QObject::connect(map, SIGNAL(clearingData()), model, SLOT(clearGroups()));
  QObject::connect(map, SIGNAL(groupAdded(const QString &)),
      model, SLOT(addGroup(const QString &)));
  QObject::connect(map, SIGNAL(removingGroup(const QString &)),
      model, SLOT(removeGroup(const QString &)));
  QObject::connect(map, SIGNAL(sourceAdded(const QString &, const QString &)),
      model, SLOT(addSource(const QString &, const QString &)));
  QObject::connect(map,
      SIGNAL(removingSource(const QString &, const QString &)),
      model, SLOT(removeSource(const QString &, const QString &)));

  map->initializeModel(model);
}

void pqPipelineBrowser::getAllowedSources(pqSourceInfoModel *model,
    vtkSMProxy *input, QStringList &list)
{
  if(!input || !model)
    {
    return;
    }

  // Get the list of available sources from the model.
  QStringList available;
  model->getAvailableSources(available);
  if(available.isEmpty())
    {
    return;
    }

  // Loop through the list of filter prototypes to find the ones that
  // are compatible with the input.
  vtkSMProxy *prototype = 0;
  vtkSMProxyProperty *prop = 0;
  vtkSMProxyManager *manager = vtkSMProxyManager::GetProxyManager();
  QStringList::Iterator iter = available.begin();
  for( ; iter != available.end(); ++iter)
    {
    prototype = manager->GetProxy("filters_prototypes",
        (*iter).toAscii().data());
    if(prototype)
      {
      prop = vtkSMProxyProperty::SafeDownCast(
          prototype->GetProperty("Input"));
      if(prop)
        {
        prop->RemoveAllUncheckedProxies();
        prop->AddUncheckedProxy(input);
        if(prop->IsInDomains())
          {
          list.append(*iter);
          }
        }
      }
    }
}


