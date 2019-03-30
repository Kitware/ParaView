/*=========================================================================

   Program: ParaView
   Module:    pqSGWritersMenuManager.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqSGWritersMenuManager.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPluginManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include <QAction>
#include <QDebug>
#include <QFile>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>

#include <cassert>

namespace
{
static vtkSMInputProperty* getInputProperty(vtkSMProxy* proxy)
{
  // if "Input" is present, we return that, otherwise the "first"
  // vtkSMInputProperty encountered is returned.

  vtkSMInputProperty* prop = vtkSMInputProperty::SafeDownCast(proxy->GetProperty("Input"));
  vtkSMPropertyIterator* propIter = proxy->NewPropertyIterator();
  for (propIter->Begin(); !prop && !propIter->IsAtEnd(); propIter->Next())
  {
    prop = vtkSMInputProperty::SafeDownCast(propIter->GetProperty());
  }

  propIter->Delete();
  return prop;
}
}

//-----------------------------------------------------------------------------
pqSGWritersMenuManager::pqSGWritersMenuManager(
  QMenu* mymenu, const char* writersMenuName, const char* objectMenuName, QObject* parentObject)
  : Superclass(parentObject)
{
  this->Menu = mymenu;
  this->AsSubMenu = false;
  this->AlreadyConnected = false;
  if (mymenu != nullptr)
  {
    this->AsSubMenu = true;
  }
  this->WritersMenuName = writersMenuName;
  this->ObjectMenuName = objectMenuName;

  // this updates the available writers whenever the active
  // source changes
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnableState()));

  // this updates the available writers whenever a filter
  // is updated (i.e. the user hits the Apply button)
  QObject::connect(&this->Timer, SIGNAL(timeout()), this, SLOT(updateEnableState()));
  this->Timer.setInterval(11);
  this->Timer.setSingleShot(true);
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(dataUpdated(pqPipelineSource*)), &this->Timer, SLOT(start()));
  QObject::connect(
    pqApplicationCore::instance(), SIGNAL(forceFilterMenuRefresh()), &this->Timer, SLOT(start()));

  // this updates the available writers whenever a plugin is
  // loaded.
  QObject::connect(pqApplicationCore::instance()->getPluginManager(), SIGNAL(pluginsUpdated()),
    this, SLOT(createMenu()));
}

//-----------------------------------------------------------------------------
pqSGWritersMenuManager::pqSGWritersMenuManager(
  const char* writersMenuName, const char* objectMenuName, QObject* parentObject)
  : pqSGWritersMenuManager(nullptr, writersMenuName, objectMenuName, parentObject)
{
}

//-----------------------------------------------------------------------------
pqSGWritersMenuManager::~pqSGWritersMenuManager()
{
}

//-----------------------------------------------------------------------------
namespace
{
QAction* findHelpMenuAction(QMenuBar* menubar)
{
  QList<QAction*> menuBarActions = menubar->actions();
  foreach (QAction* existingMenuAction, menuBarActions)
  {
    QString menuName = existingMenuAction->text().toLower();
    menuName.remove('&');
    if (menuName == "help")
    {
      return existingMenuAction;
    }
  }
  return NULL;
}
}

//-----------------------------------------------------------------------------
void pqSGWritersMenuManager::createMenu()
{
  QMainWindow* mainWindow = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
  if (mainWindow == NULL)
  {
    // we don't have a main window to add our menus to yet so we'll wait a bit
    QTimer::singleShot(1000, this, SLOT(createMenu()));
    return;
  }

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if (pxm == NULL)
  {
    return;
  }

  // in rebuild after plugin case, find the submenu to add under
  QMenu* submenu = nullptr;
  QList<QAction*> menu_actions = this->Menu->findChildren<QAction*>();
  foreach (QAction* action, menu_actions)
  {
    submenu = action->menu();
    // remove potential ampersand (&) that can be added to the title by Qt
    if (submenu && (submenu->title().replace('&', "") == tr("Data Extract Writers -deprecated")))
    {
      break;
    }
    submenu = nullptr;
  }

  // connect writer menu item actions to createWriters() method
  if (this->AsSubMenu)
  {
    if (!this->AlreadyConnected)
    {
      // make the submenu
      submenu = new QMenu(tr("Data Extract Writers -deprecated"), this->Menu);
      submenu->setObjectName("Writers");
      this->Menu->addMenu(submenu);
      // connect its actions up
      QObject::connect(submenu, SIGNAL(triggered(QAction*)), this,
        SLOT(onActionTriggered(QAction*)), Qt::QueuedConnection);
    }
    if (submenu)
    {
      submenu->clear();
    }
  }
  else
  {
    if (this->Menu == NULL)
    {
      this->Menu = new QMenu(this->WritersMenuName, mainWindow);
      this->Menu->setObjectName(this->ObjectMenuName);
      mainWindow->menuBar()->insertMenu(::findHelpMenuAction(mainWindow->menuBar()), this->Menu);

      QObject::connect(this->Menu, SIGNAL(triggered(QAction*)), this,
        SLOT(onActionTriggered(QAction*)), Qt::QueuedConnection);
    }
    this->Menu->clear();
  }

  vtkSMProxyDefinitionManager* proxyDefinitions = pxm->GetProxyDefinitionManager();

  // For search proxies in the insitu_writer_parameters group and
  // we search specifically for proxies with a proxy writer hint
  // since we've marked them as special
  QMenu* addTo = submenu ? submenu : this->Menu;
  const char proxyGroup[] = "insitu_writer_parameters";
  vtkPVProxyDefinitionIterator* iter = proxyDefinitions->NewSingleGroupIterator(proxyGroup);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (vtkPVXMLElement* hints = iter->GetProxyHints())
    {
      if (hints->FindNestedElementByName("WriterProxy"))
      {
        const char* proxyName = iter->GetProxyName();
        vtkSMProxy* prototype = pxm->GetPrototypeProxy(proxyGroup, proxyName);
        if (!prototype)
        {
          qWarning() << "Failed to locate proxy for writer: " << proxyGroup << " , " << proxyName;
          continue;
        }
        QAction* action = addTo->addAction(
          prototype->GetXMLLabel() ? prototype->GetXMLLabel() : prototype->GetXMLName());
        QStringList list;
        list << proxyGroup << proxyName;
        action->setData(list);
      }
    }
  }
  iter->Delete();

  this->updateEnableState();

  this->AlreadyConnected = true;
}

//-----------------------------------------------------------------------------
void pqSGWritersMenuManager::updateEnableState()
{
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if (!this->Menu || !pxm)
  {
    return;
  }

  // Get the list of selected sources. Make sure the list contains
  // only valid sources.
  pqServerManagerModel* pqModel = pqApplicationCore::instance()->getServerManagerModel();
  vtkSMProxySelectionModel* selection = pxm->GetSelectionModel("ActiveSources");

  QList<pqOutputPort*> outputPorts;
  for (unsigned int index = 0; index < selection->GetNumberOfSelectedProxies(); ++index)
  {
    vtkSMProxy* smProxy = selection->GetSelectedProxy(index);
    pqPipelineSource* source = pqModel->findItem<pqPipelineSource*>(smProxy);
    pqOutputPort* port =
      source ? source->getOutputPort(0) : pqModel->findItem<pqOutputPort*>(smProxy);
    if (port)
    {
      outputPorts.append(port);
    }
  }

  assert("A proxy manager should have been found by now" && pxm);

  // Iterate over all filters in the menu and see if they can be
  // applied to the current source(s).
  bool some_enabled = false;
  QList<QAction*> menu_actions = this->Menu->findChildren<QAction*>();
  foreach (QAction* action, menu_actions)
  {
    QStringList filterType = action->data().toStringList();
    if (filterType.size() != 2)
    {
      continue;
    }
    if (filterType[0] != "insitu_writer_parameters")
    {
      continue;
    }
    if (outputPorts.size() == 0)
    {
      action->setEnabled(false);
      continue;
    }

    vtkSMProxy* output = pxm->GetPrototypeProxy(
      filterType[0].toLocal8Bit().data(), filterType[1].toLocal8Bit().data());
    if (!output)
    {
      action->setEnabled(false);
      continue;
    }

    int numProcs = outputPorts[0]->getServer()->getNumberOfPartitions();
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(output);
    if (sp && ((sp->GetProcessSupport() == vtkSMSourceProxy::SINGLE_PROCESS && numProcs > 1) ||
                (sp->GetProcessSupport() == vtkSMSourceProxy::MULTIPLE_PROCESSES && numProcs == 1)))
    {
      // Skip single process filters when running in multiprocesses and vice
      // versa.
      action->setEnabled(false);
      continue;
    }

    vtkSMInputProperty* input = ::getInputProperty(output);
    if (input)
    {
      if (!input->GetMultipleInput() && selection->GetNumberOfSelectedProxies() > 1)
      {
        action->setEnabled(false);
        continue;
      }

      input->RemoveAllUncheckedProxies();
      for (int cc = 0; cc < outputPorts.size(); cc++)
      {
        pqOutputPort* port = outputPorts[cc];
        input->AddUncheckedInputConnection(port->getSource()->getProxy(), port->getPortNumber());
      }

      if (input->IsInDomains())
      {
        action->setEnabled(true);
        some_enabled = true;
      }
      else
      {
        action->setEnabled(false);
      }
      input->RemoveAllUncheckedProxies();
    }
  }

  if (!this->AsSubMenu)
  {
    this->Menu->setEnabled(some_enabled);
  }
}

//-----------------------------------------------------------------------------
void pqSGWritersMenuManager::onActionTriggered(QAction* action)
{
  QStringList filterType = action->data().toStringList();
  if (filterType.size() == 2)
  {
    if (filterType[0] != "insitu_writer_parameters")
    {
      return;
    }
    this->createWriter(filterType[0], filterType[1]);
  }
}

//-----------------------------------------------------------------------------
void pqSGWritersMenuManager::createWriter(const QString& xmlgroup, const QString& xmlname)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  vtkSMProxy* prototype =
    pxm->GetPrototypeProxy(xmlgroup.toLocal8Bit().data(), xmlname.toLocal8Bit().data());
  if (!prototype)
  {
    qCritical() << "Unknown proxy type: " << xmlname;
    return;
  }

  // Get the list of selected sources.
  vtkSMProxySelectionModel* selection = pxm->GetSelectionModel("ActiveSources");
  pqServerManagerModel* pqModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqOutputPort*> selectedOutputPorts;
  QMap<QString, QList<pqOutputPort*> > namedInputs;
  // Determine the list of selected output ports.
  for (unsigned int index = 0; index < selection->GetNumberOfSelectedProxies(); ++index)
  {
    if (pqOutputPort* opPort = pqModel->findItem<pqOutputPort*>(selection->GetSelectedProxy(index)))
    {
      selectedOutputPorts.push_back(opPort);
    }
    else if (pqPipelineSource* source =
               pqModel->findItem<pqPipelineSource*>(selection->GetSelectedProxy(index)))
    {
      selectedOutputPorts.push_back(source->getOutputPort(0));
    }
  }

  QList<const char*> inputPortNames = pqPipelineFilter::getInputPorts(prototype);
  namedInputs[inputPortNames[0]] = selectedOutputPorts;

  pqApplicationCore::instance()->getUndoStack()->beginUndoSet(QString("Create '%1'").arg(xmlname));
  pqPipelineSource* filter =
    builder->createFilter(xmlgroup, xmlname, namedInputs, selectedOutputPorts[0]->getServer());
  (void)filter;
  pqApplicationCore::instance()->getUndoStack()->endUndoSet();
}
