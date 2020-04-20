/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
  foreach (QObject* iface, this->Interfaces)
  {
    pqAutoStartInterface* asi = qobject_cast<pqAutoStartInterface*>(iface);
    if (asi)
    {
      asi->shutdown();
    }
  }

  foreach (QObject* iface, this->RegisteredInterfaces)
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
    this->onPluginLoaded(NULL, 0, tracker->GetPlugin(cc));
  }
}

//-----------------------------------------------------------------------------
void pqInterfaceTracker::onPluginLoaded(vtkObject*, unsigned long, void* calldata)
{
  vtkPVPlugin* plugin = reinterpret_cast<vtkPVPlugin*>(calldata);
  vtkPVGUIPluginInterface* guiplugin = dynamic_cast<vtkPVGUIPluginInterface*>(plugin);
  if (guiplugin != NULL)
  {
    QObjectList ifaces = guiplugin->interfaces();
    foreach (QObject* iface, ifaces)
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
