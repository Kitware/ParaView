/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBrowser.cxx

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

/// \file pqPipelineBrowser.cxx
/// \date 4/20/2006

#include "pqPipelineBrowser.h"

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqDisplayPolicy.h"
#include "pqFilterInputDialog.h"
#include "pqFlatTreeView.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineBrowserStateManager.h"
#include "pqPipelineFilter.h"
#include "pqPipelineModel.h"
#include "pqPipelineModelSelectionAdaptor.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
//#include "pqSourceInfoIcons.h"
//#include "pqSourceHistoryModel.h"
//#include "pqSourceInfoFilterModel.h"
//#include "pqSourceInfoGroupMap.h"
//#include "pqSourceInfoModel.h"
//#include "pqAddSourceDialog.h"

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
#include "vtkSMInputProperty.h"


class pqPipelineBrowserInternal
{
public:
  pqPipelineBrowserInternal();
  ~pqPipelineBrowserInternal() {}

  // TODO: Add support for multiple servers.
  //pqSourceInfoModel *FilterModel;
  QString LastFilterGroup;
  QPointer<pqView> View;
};


//----------------------------------------------------------------------------
pqPipelineBrowserInternal::pqPipelineBrowserInternal()
  : LastFilterGroup()
{
  //this->FilterModel = 0;
  this->View = 0;
}


//----------------------------------------------------------------------------
pqPipelineBrowser::pqPipelineBrowser(QWidget *widgetParent)
  : QWidget(widgetParent)
{
  this->Internal = new pqPipelineBrowserInternal();
  this->Model = 0;
  this->TreeView = 0;
  //this->Icons = new pqSourceInfoIcons(this);
  //this->FilterGroups = new pqSourceInfoGroupMap(this);
  //this->FilterHistory = new pqSourceHistoryModel(this);
  this->Manager = new pqPipelineBrowserStateManager(this);

  // Set the icons for the history models.
  //this->FilterHistory->setIcons(this->Icons, pqSourceInfoIcons::Filter);

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
      SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*, int)),
      this->Model,
      SLOT(addConnection(pqPipelineSource*, pqPipelineSource*, int)));
  this->connect(smModel,
      SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*, int)),
      this->Model,
      SLOT(removeConnection(pqPipelineSource*, pqPipelineSource*, int)));

  this->connect(smModel, SIGNAL(nameChanged(pqServerManagerModelItem *)),
      this->Model, SLOT(updateItemName(pqServerManagerModelItem *)));
  
  this->connect(
    this->Model, SIGNAL(rename(const QModelIndex&, const QString&)),
    this, SLOT(onRename(const QModelIndex&, const QString&)));

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

//----------------------------------------------------------------------------
pqPipelineBrowser::~pqPipelineBrowser()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
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

#if 0
//----------------------------------------------------------------------------
void pqPipelineBrowser::loadFilterInfo(vtkPVXMLElement *root)
{
  this->FilterGroups->loadSourceInfo(root);

  // TEMP: Add in the list of released filters.
  this->FilterGroups->addGroup("Released");
  this->FilterGroups->addSource("Clip", "Released");
  this->FilterGroups->addSource("Cut", "Released");
  this->FilterGroups->addSource("Threshold", "Released");
}
#endif

//----------------------------------------------------------------------------
void pqPipelineBrowser::saveState(vtkPVXMLElement *root) const
{
  this->Manager->saveState(root);
}

//----------------------------------------------------------------------------
void pqPipelineBrowser::restoreState(vtkPVXMLElement *root)
{
  this->Manager->restoreState(root);
}

//----------------------------------------------------------------------------
QItemSelectionModel *pqPipelineBrowser::getSelectionModel() const
{
  return this->TreeView->getSelectionModel();
}

//----------------------------------------------------------------------------
pqView *pqPipelineBrowser::getView() const
{
  return this->Internal->View;
}

#if 0
//----------------------------------------------------------------------------
void pqPipelineBrowser::addSource()
{
  // TODO
}

//----------------------------------------------------------------------------
void pqPipelineBrowser::addFilter()
{
  // Get the source input(s) from the browser's selection model.
  QModelIndexList indexes = this->getSelectionModel()->selectedIndexes();
  if(indexes.size() < 1)
    {
    return;
    }

  // Get the filter info model for the current server.
  pqSourceInfoModel *model = this->getFilterModel();

  // Use a proxy model to display only the allowed filters.
  QStringList allowed;
  this->getAllowedSources(model, indexes, allowed);
  pqSourceInfoFilterModel *modelFilter = new pqSourceInfoFilterModel(this);
  modelFilter->setSourceModel(model);
  modelFilter->setAllowedNames(allowed);

  pqSourceInfoFilterModel *history = new pqSourceInfoFilterModel(this);
  history->setSourceModel(this->FilterHistory);
  history->setAllowedNames(allowed);

  // Set up the add filter dialog.
  pqAddSourceDialog dialog(QApplication::activeWindow());
  dialog.setSourceMap(this->FilterGroups);
  dialog.setSourceList(modelFilter);
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
    pqApplicationCore *core = pqApplicationCore::instance();
    pqObjectBuilder* builder = core->getObjectBuilder();

    QList<pqPipelineSource*> inputs;
    for(QModelIndexList::Iterator index = indexes.begin();
      index != indexes.end(); ++index)
      {
      pqPipelineSource* source = dynamic_cast<pqPipelineSource *>(
          this->Model->getItemFor(*index));
      if (source)
        {
        inputs.push_back(source);
        }
      }

    pqPipelineSource *filter = builder->createFilter("filters", filterName,
      inputs);
    if(!filter)
      {
      qCritical() << "Filter could not be created.";
      }
    }

  delete modelFilter;
  delete history;
}
#endif

//----------------------------------------------------------------------------
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
    pqFilterInputDialog dialog(this);
    dialog.setObjectName("ChangeInputDialog");
    pqServerManagerModel *smModel =
        pqApplicationCore::instance()->getServerManagerModel();
    pqPipelineModel *model = new pqPipelineModel(*smModel);
    dialog.setModelAndFilter(model, filter, filter->getNamedInputs());
    if(QDialog::Accepted == dialog.exec())
      {
      emit this->beginUndo(QString("Change Input for %1").arg(
          filter->getSMName()));
      for (int cc=0; cc < filter->getNumberOfInputPorts(); cc++)
        {
        QString inputPortName = filter->getInputPortName(cc);
        QList<pqOutputPort*> inputs = dialog.getFilterInputs(inputPortName);

        vtkstd::vector<vtkSMProxy*> inputPtrs;
        vtkstd::vector<unsigned int> inputPorts;

        foreach (pqOutputPort* opport, inputs)
          {
          inputPtrs.push_back(opport->getSource()->getProxy());
          inputPorts.push_back(opport->getPortNumber());
          }

        vtkSMInputProperty* ip =vtkSMInputProperty::SafeDownCast(
          filter->getProxy()->GetProperty(
            inputPortName.toAscii().data()));
        ip->SetProxies(inputPtrs.size(), &inputPtrs[0], &inputPorts[0]);
        }
      filter->getProxy()->UpdateVTKObjects();
      emit this->endUndo();

      // render all views
      pqApplicationCore::instance()->render();
      }

    delete model;
    }
}

//----------------------------------------------------------------------------
void pqPipelineBrowser::deleteSelected()
{
  QModelIndexList indexes = this->getSelectionModel()->selectedIndexes();
  if (indexes.size() <= 0)
    {
    return;
    }

  if (indexes.size() > 1)
    {
    QSet<pqPipelineSource*> selectedSources;
    foreach (QModelIndex index, indexes)
      {
      pqPipelineSource *source = qobject_cast<pqPipelineSource*>(
        this->Model->getItemFor(index));
      if (source)
        {
        selectedSources.insert(source);
        }
      }

    // multiple item delete.
    emit this->beginUndo(QString("Delete Selection"));
    bool something_deleted = true;
    while (something_deleted)
      {
      something_deleted = false;
      foreach (pqPipelineSource* source, selectedSources)
        {
        if (source && source->getNumberOfConsumers() == 0)
          {
          selectedSources.remove(source);
          pqApplicationCore::instance()->getObjectBuilder()->destroy(source);
          something_deleted = true;
          }
        }
      }
    emit this->endUndo();
    }
  else if (indexes.size() == 1)
    {
    // Get the selected item(s) from the selection model.
    pqServerManagerModelItem *item = this->Model->getItemFor(indexes.first());
    pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
    pqServer *server = dynamic_cast<pqServer *>(item);
    if(source)
      {
      if(source->getNumberOfConsumers() == 0)
        {
        emit this->beginUndo(QString("Delete %1").arg(source->getSMName()));
        pqApplicationCore::instance()->getObjectBuilder()->destroy(source);
        emit this->endUndo();
        }
      }
    else if(server)
      {
      pqApplicationCore::instance()->getObjectBuilder()->removeServer(server);
      }
    }
}

//----------------------------------------------------------------------------
void pqPipelineBrowser::onRename(const QModelIndex& index, const QString& name)
{
  pqPipelineSource* source = qobject_cast<pqPipelineSource*>(
    this->Model->getItemFor(index));
  if (source)
    {
    emit this->beginUndo(QString("Rename %1 to %2").arg(
        source->getSMName()).arg(name));
    source->rename(name);
    emit this->endUndo();
    }
}

//----------------------------------------------------------------------------
void pqPipelineBrowser::setView(pqView *rm)
{
  this->Internal->View= rm;
  this->Model->setView(rm);
}

//----------------------------------------------------------------------------
void pqPipelineBrowser::handleIndexClicked(const QModelIndex &index)
{
  QModelIndexList indexes = this->getSelectionModel()->selectedIndexes();
  if (indexes.size() > 1 && index.column() == 1)
    {
    //have multiple items selected, and we are clicking on an eye
    //now we need to detect if the eye we are clicking on is one that is selected
    
    pqServerManagerModelItem *smIndex, *smItem;
    QModelIndex item;
    
    smIndex = this->Model->getItemFor(index);
    bool inSelection = false;    
    for (int i=0; i < indexes.size() && !inSelection; i++)
      {            
      smItem = this->Model->getItemFor(indexes.at(i));      
      if (smItem == smIndex)
        {              
        inSelection = true;        
        }
      }
    //the eye we clicked on is in the selected group, so operate on the entire group
    if (inSelection)
      {
      emit this->beginUndo("Change Visibility of multiple items");
      for (int i=0; i < indexes.size(); i++)
        {        
        item = indexes.at(i);
        pqPipelineModel::ItemType itemType;
        itemType = this->Model->getTypeFor(item);
        //ignore link items as that solves the massive problem
        //of selecting multiple links and setting visibility multiple times on the same item
        if (itemType != pqPipelineModel::Link)    
          {
          this->handleSingleClickItem(item);
          }
        }
      emit this->endUndo();
      return;
      }

    // we will just do visibility on the eye that was clicked, since it is not
    // part of the main group -- simply fall through.
    }

  // we make sure we are only clicking on an eye
  // and we have 1 or less items selected
  if (index.column() == 1)
    {
    // We need to obtain the source to give the undo element some sensible name.
    pqServerManagerModelItem* smModelItem = this->Model->getItemFor(index);
    pqPipelineSource *source = qobject_cast<pqPipelineSource*>(smModelItem);
    pqOutputPort* port = source? source->getOutputPort(0) :
      qobject_cast<pqOutputPort*>(smModelItem);
    source = port->getSource();

    // call the old eye code
    emit this->beginUndo(QString("Change Visibility of %1").arg(
        source->getSMName()));
    this->handleSingleClickItem(index); 
    emit this->endUndo();
    }
}
//----------------------------------------------------------------------------
void pqPipelineBrowser::handleSingleClickItem(const QModelIndex &index)
{
  // See if the index is associated with a source.
  pqServerManagerModelItem* smModelItem = this->Model->getItemFor(index);

  pqPipelineSource *source = qobject_cast<pqPipelineSource*>(smModelItem);
  pqOutputPort* port = source? source->getOutputPort(0) :
    qobject_cast<pqOutputPort*>(smModelItem);
  if(port)
    {
    source = port->getSource();

    if (source->modifiedState() != pqProxy::UNINITIALIZED)
      {
      // If the column clicked is 1, the user clicked the visible icon.
      // Get the display object for the current window.
      pqDataRepresentation* repr = port->getRepresentation(this->Internal->View);

      bool visible = true;
      // If the display exists, toggle the display. Otherwise, create a
      // display for the source in the current window.
      if(repr)
        {
        visible = !repr->isVisible();
        }

      pqDisplayPolicy* dpolicy = 
        pqApplicationCore::instance()->getDisplayPolicy();
      // Will create new display if needed. May also create new view 
      // as defined by the policy.
      repr = dpolicy->setRepresentationVisibility(
        port, this->Internal->View, visible);

      if(repr)
        {
        repr->renderView(false);
        }

      // Change the selection to the item if we just made it visible.
      if (visible)
        {
        QModelIndex nameIndex
          = this->Model->index(index.row(), 0, index.parent());
        this->getSelectionModel()->select(nameIndex,
                                          QItemSelectionModel::ClearAndSelect);
        }
      }
    // TODO
    //else if(index.column() == 2)
    //  {
    //  // If the column clicked is 2, the user clicked the input menu.
    //  }
    }
}

#if 0
//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void pqPipelineBrowser::getAllowedSources(pqSourceInfoModel *model,
    const QModelIndexList &indexes, QStringList &list)
{
  if(!model || indexes.size() == 0)
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

  // Convert the list of indexes to a list of sources.
  QList<pqPipelineSource *> sources;
  QModelIndexList::ConstIterator index = indexes.begin();
  for( ; index != indexes.end(); ++index)
    {
    pqPipelineSource *item = dynamic_cast<pqPipelineSource *>(
        this->Model->getItemFor(*index));
    if(item)
      {
      sources.append(item);
      }
    }

  if(sources.size() == 0)
    {
    return;
    }

  // Loop through the list of filter prototypes to find the ones that
  // are compatible with the input.
  vtkSMProxy *prototype = 0;
  vtkSMInputProperty *prop = 0;
  vtkSMProxyManager *manager = vtkSMProxyManager::GetProxyManager();
  QStringList::Iterator iter = available.begin();
  for( ; iter != available.end(); ++iter)
    {
    prototype = manager->GetProxy("filters_prototypes",
        (*iter).toAscii().data());
    if(prototype)
      {
      prop = vtkSMInputProperty::SafeDownCast(prototype->GetProperty("Input"));
      if(prop)
        {
        if(sources.size() > 1 && !prop->GetMultipleInput())
          {
          continue;
          }

        prop->RemoveAllUncheckedProxies();
        QList<pqPipelineSource *>::Iterator source = sources.begin();
        for( ; source != sources.end(); ++source)
          {
          prop->AddUncheckedProxy((*source)->getProxy());
          }

        if(prop->IsInDomains())
          {
          list.append(*iter);
          }
        }
      }
    }
}
#endif


