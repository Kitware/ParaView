/*=========================================================================

   Program: ParaView
   Module:    pqPipelineMenu.cxx

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

========================================================================*/

/// \file pqPipelineMenu.cxx
/// \date 6/5/2006

#include "pqPipelineMenu.h"

#include "pqAddSourceDialog.h"
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqSourceHistoryModel.h"
#include "pqSourceInfoFilterModel.h"
#include "pqSourceInfoGroupMap.h"
#include "pqSourceInfoIcons.h"
#include "pqSourceInfoModel.h"

#include <QAction>
#include <QApplication>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QStringList>
#include <QtDebug>
#include <QWidget>

#include "vtkPVXMLElement.h"
#include "vtkSMObject.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"


class pqPipelineMenuInternal
{
public:
  pqPipelineMenuInternal();
  ~pqPipelineMenuInternal() {}

  pqSourceInfoIcons *Icons;

  pqSourceInfoGroupMap *FilterGroups;
  pqSourceHistoryModel *FilterHistory;

  // TODO: Add support for multiple servers.
  pqSourceInfoModel *FilterModel;

  QString LastFilterGroup;
};

pqPipelineMenuInternal::pqPipelineMenuInternal()
  : LastFilterGroup()
{
  this->Icons = 0;
  this->FilterGroups = 0;
  this->FilterHistory = 0;
  this->FilterModel = 0;
}


pqPipelineMenu::pqPipelineMenu(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqPipelineMenuInternal();
  this->Internal->Icons = new pqSourceInfoIcons(this);
  this->Internal->FilterGroups = new pqSourceInfoGroupMap(this);
  this->Internal->FilterHistory = new pqSourceHistoryModel(this);

  // Set the icons for the history models.
  this->Internal->FilterHistory->setIcons(this->Internal->Icons,
      pqSourceInfoIcons::Filter);

  // Initialize the pipeline menu actions.
  QAction *action = 0;
  this->MenuList = new QAction *[pqPipelineMenu::LastAction + 1];
  this->MenuList[pqPipelineMenu::AddSourceAction] = 0;
  action = new QAction(tr("Add &Filter..."), this);
  action->setObjectName("AddFilter");
  QObject::connect(action, SIGNAL(triggered()), this, SIGNAL(filtersActivated()));
  this->MenuList[pqPipelineMenu::AddFilterAction] = action;

  // TEMP: Set the add filter start path to the 'Released' group.
  this->Internal->LastFilterGroup = "Released";
}

pqPipelineMenu::~pqPipelineMenu()
{
  // The icon map, group maps, and models will get cleaned up by Qt.
  delete this->Internal;
  delete [] this->MenuList;
}

pqSourceInfoIcons *pqPipelineMenu::getIcons() const
{
  return this->Internal->Icons;
}

void pqPipelineMenu::loadSourceInfo(vtkPVXMLElement *)
{
  // TODO
}

void pqPipelineMenu::loadFilterInfo(vtkPVXMLElement *root)
{
  this->Internal->FilterGroups->loadSourceInfo(root);

  // TEMP: Add in the list of released filters.
  this->Internal->FilterGroups->addGroup("Released");
  this->Internal->FilterGroups->addSource("Clip", "Released");
  this->Internal->FilterGroups->addSource("Cut", "Released");
  this->Internal->FilterGroups->addSource("Threshold", "Released");
}

pqSourceInfoModel *pqPipelineMenu::getFilterModel()
{
  // TODO: Add support for multiple servers.
  if(!this->Internal->FilterModel)
    {
    // Get the list of available filters from the server manager.
    QStringList filters;
    vtkSMProxyManager *manager = vtkSMObject::GetProxyManager();
    manager->InstantiateGroupPrototypes("filters");
    unsigned int total = manager->GetNumberOfProxies("filters_prototypes");
    for(unsigned int i = 0; i < total; i++)
      {
      filters.append(manager->GetProxyName("filters_prototypes", i));
      }

    // Create a new model for the filter groups.
    this->Internal->FilterModel = new pqSourceInfoModel(filters, this);

    // Initialize the new model.
    this->setupConnections(this->Internal->FilterModel,
        this->Internal->FilterGroups);
    this->Internal->FilterModel->setIcons(this->Internal->Icons,
        pqSourceInfoIcons::Filter);
    }

  return this->Internal->FilterModel;
}

void pqPipelineMenu::addActionsToMenuBar(QMenuBar *menubar) const
{
  if(menubar)
    {
    QMenu *menu = menubar->addMenu(tr("&Pipeline"));
    menu->setObjectName("PipelineMenu");
    this->addActionsToMenu(menu);
    }
}

void pqPipelineMenu::addActionsToMenu(QMenu *menu) const
{
  if(!menu)
    {
    return;
    }

  // TODO: Finish this
  menu->addAction(this->MenuList[pqPipelineMenu::AddFilterAction]);
}

QAction *pqPipelineMenu::getMenuAction(pqPipelineMenu::ActionName name) const
{
  if(name != pqPipelineMenu::InvalidAction)
    {
    return this->MenuList[name];
    }

  return 0;
}

void pqPipelineMenu::addSource()
{
  // TODO
}

void pqPipelineMenu::addFilter(pqPipelineSource* input)
{
  if(!input)
    {
    return;
    }

  // Get the filter info model for the current server.
  pqSourceInfoModel *model = this->getFilterModel();

  // Use a proxy model to display only the allowed filters.
  QStringList allowed;
  pqSourceInfoFilterModel *filter = new pqSourceInfoFilterModel(this);
  filter->setSourceModel(model);
  this->getAllowedSources(model, input->getProxy(), allowed);
  filter->setAllowedNames(allowed);

  pqSourceInfoFilterModel *history = new pqSourceInfoFilterModel(this);
  history->setSourceModel(this->Internal->FilterHistory);
  history->setAllowedNames(allowed);

  // Set up the add filter dialog.
  pqAddSourceDialog dialog(QApplication::activeWindow());
  dialog.setSourceMap(this->Internal->FilterGroups);
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
    this->Internal->FilterHistory->addRecentSource(filterName);

    // Create the filter.
    if(!pqApplicationCore::instance()->createFilterForSource(filterName, input))
      {
      qCritical() << "Filter could not be created.";
      } 
    }

  delete filter;
  delete history;
}

void pqPipelineMenu::setupConnections(pqSourceInfoModel *model,
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

void pqPipelineMenu::getAllowedSources(pqSourceInfoModel *model,
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
  vtkSMProxyManager *manager = vtkSMObject::GetProxyManager();
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


