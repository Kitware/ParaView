
/// \file pqPipelineMenu.cxx
/// \date 6/5/2006

#include "pqPipelineMenu.h"

#include "pqAddSourceDialog.h"
#include "pqApplicationCore.h"
#include "pqSourceInfoModel.h"
#include "pqSourceInfoGroupMap.h"

#include <QAction>
#include <QApplication>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QStringList>
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

  pqSourceInfoGroupMap *FilterGroups;

  // TODO: Add support for multiple connections.
  pqSourceInfoModel *FilterModel;
};

pqPipelineMenuInternal::pqPipelineMenuInternal()
{
  this->FilterGroups = 0;
  this->FilterModel = 0;
}


pqPipelineMenu::pqPipelineMenu(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqPipelineMenuInternal();
  this->Internal->FilterGroups = new pqSourceInfoGroupMap(this);

  // Initialize the pipeline menu actions.
  QAction *action = 0;
  this->MenuList = new QAction *[pqPipelineMenu::LastAction + 1];
  this->MenuList[pqPipelineMenu::AddSourceAction] = 0;
  action = new QAction(tr("Add &Filter..."), this);
  action->setObjectName("AddFilter");
  QObject::connect(action, SIGNAL(triggered()), this, SLOT(addFilter()));
  this->MenuList[pqPipelineMenu::AddFilterAction] = action;
  this->MenuList[pqPipelineMenu::AddBundleAction] = 0;
}

pqPipelineMenu::~pqPipelineMenu()
{
  delete this->Internal->FilterGroups;
  delete this->Internal->FilterModel;
  delete this->Internal;
  delete [] this->MenuList;
}

void pqPipelineMenu::loadSourceInfo(vtkPVXMLElement *)
{
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

void pqPipelineMenu::loadBundleInfo(vtkPVXMLElement *)
{
}

pqSourceInfoModel *pqPipelineMenu::getFilterModel()
{
  // TODO: Add support for multiple connections.
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
    }

  return this->Internal->FilterModel;
}

void pqPipelineMenu::addActionsToMenuBar(QMenuBar *menubar) const
{
  if(menubar)
    {
    QMenu *menu = menubar->addMenu(tr("&Pipeline"));
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
}

void pqPipelineMenu::addFilter()
{
  // Get the filter info model for the current server.
  pqSourceInfoModel *model = this->getFilterModel();

  // TODO: Use a proxy model to display only the allowed filters.

  // Set up the add filter dialog.
  pqAddSourceDialog dialog(QApplication::activeWindow());
  dialog.setSourceList(model);

  // TEMP: Start the dialog in the 'Released' group.
  // TODO: Start the user in the previous group path.
  dialog.setPath("Released");
  if(dialog.exec() == QDialog::Accepted)
    {
    // If the user selects a filter, save the starting path and emit
    // a signal.
    }
}

void pqPipelineMenu::addBundle()
{
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

  // TODO: Get the list of available sources from the model.
  QStringList available;
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


