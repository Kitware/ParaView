// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqInterfaceTracker.h"

#include "pqAutoStartInterface.h"
#include "vtkCommand.h"
#include "vtkPVGUIPluginInterface.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginTracker.h"

//-----------------------------------------------------------------------------
pqInterfaceTracker::pqInterfaceTracker(QObject* parentObject)
  : Superclass(parentObject)
{
  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();

  // Register with vtkPVPluginTracker so that when new plugin is loaded, we can
  // locate and load any Qt-interface implementations provided by the plugin.
  this->ObserverID =
    tracker->AddObserver(vtkCommand::RegisterEvent, this, &pqInterfaceTracker::onPluginLoaded);
}

//-----------------------------------------------------------------------------
pqInterfaceTracker::~pqInterfaceTracker()
{
  // Shutdown all autostart interfaces.
  Q_FOREACH (QObject* iface, this->Interfaces)
  {
    pqAutoStartInterface* asi = qobject_cast<pqAutoStartInterface*>(iface);
    if (asi)
    {
      asi->shutdown();
    }
  }

  Q_FOREACH (QObject* iface, this->RegisteredInterfaces)
  {
    pqAutoStartInterface* asi = qobject_cast<pqAutoStartInterface*>(iface);
    if (asi)
    {
      asi->shutdown();
    }
  }

  vtkPVPluginTracker::GetInstance()->RemoveObserver(this->ObserverID);
}

//-----------------------------------------------------------------------------
void pqInterfaceTracker::initialize()
{
  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();

  // Process any already loaded plugins. These are typically the plugins that
  // got autoloaded when the application started.
  for (unsigned int cc = 0; cc < tracker->GetNumberOfPlugins(); cc++)
  {
    this->onPluginLoaded(nullptr, 0, tracker->GetPlugin(cc));
  }
}

//-----------------------------------------------------------------------------
void pqInterfaceTracker::onPluginLoaded(vtkObject*, unsigned long, void* calldata)
{
  vtkPVPlugin* plugin = reinterpret_cast<vtkPVPlugin*>(calldata);
  vtkPVGUIPluginInterface* guiplugin = dynamic_cast<vtkPVGUIPluginInterface*>(plugin);
  if (guiplugin != nullptr)
  {
    QObjectList ifaces = guiplugin->interfaces();
    Q_FOREACH (QObject* iface, ifaces)
    {
      this->Interfaces.append(iface);
      Q_EMIT this->interfaceRegistered(iface);

      pqAutoStartInterface* asi = qobject_cast<pqAutoStartInterface*>(iface);
      if (asi)
      {
        asi->startup();
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqInterfaceTracker::addInterface(QObject* iface)
{
  if (!this->Interfaces.contains(iface) && !this->RegisteredInterfaces.contains(iface))
  {
    this->RegisteredInterfaces.push_back(iface);
    Q_EMIT this->interfaceRegistered(iface);

    pqAutoStartInterface* asi = qobject_cast<pqAutoStartInterface*>(iface);
    if (asi)
    {
      asi->startup();
    }
  }
}

//-----------------------------------------------------------------------------
void pqInterfaceTracker::removeInterface(QObject* iface)
{
  int idx = this->RegisteredInterfaces.indexOf(iface);
  if (idx != -1)
  {
    this->RegisteredInterfaces.removeAt(idx);
    pqAutoStartInterface* asi = qobject_cast<pqAutoStartInterface*>(iface);
    if (asi)
    {
      asi->shutdown();
    }
  }
}
