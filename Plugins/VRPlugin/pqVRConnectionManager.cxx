/*=========================================================================

  Program: ParaView
  Module:  vtkVRConnectionManager.cxx

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

=========================================================================*/
#include "pqVRConnectionManager.h"

// --------------------------------------------------------------------includes
#include "pqApplicationCore.h"
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
#include "pqVRPNConnection.h"
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
#include "pqVRUIConnection.h"
#endif

#include "vtkObjectFactory.h"
#include "vtkPVVRConfig.h"
#include "vtkPVXMLElement.h"
#include "vtkVRQueue.h"
#include "vtkWeakPointer.h"

#include <QList>
#include <QtDebug>

#include <cassert>

// ----------------------------------------------------------------------------
QPointer<pqVRConnectionManager> pqVRConnectionManager::Instance;
void pqVRConnectionManager::setInstance(pqVRConnectionManager* mgr)
{
  pqVRConnectionManager::Instance = mgr;
}

// ----------------------------------------------------------------------------
pqVRConnectionManager* pqVRConnectionManager::instance()
{
  return pqVRConnectionManager::Instance;
}

// --------------------------------------------------------------------internal
// IMPORTANT: Make sure that this struct has no pointers.  All pointers should
// be put in the class declaration. For all newly defined pointers make sure to
// update constructor and destructor methods.
struct pqVRConnectionManager::pqInternals
{
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  QList<QPointer<pqVRPNConnection> > VRPNConnections;
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
  QList<QPointer<pqVRUIConnection> > VRUIConnections;
#endif
  vtkWeakPointer<vtkVRQueue> Queue;
};

// -----------------------------------------------------------------------cnstr
pqVRConnectionManager::pqVRConnectionManager(vtkVRQueue* queue, QObject* parentObject)
  : Superclass(parentObject)
{
  this->Internals = new pqInternals();
  this->Internals->Queue = queue;
  QObject::connect(pqApplicationCore::instance(),
    SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)), this,
    SLOT(configureConnections(vtkPVXMLElement*, vtkSMProxyLocator*)));
  QObject::connect(pqApplicationCore::instance(), SIGNAL(stateSaved(vtkPVXMLElement*)), this,
    SLOT(saveConnectionsConfiguration(vtkPVXMLElement*)));
}

// -----------------------------------------------------------------------destr
pqVRConnectionManager::~pqVRConnectionManager()
{
  delete this->Internals;
}

#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
// ----------------------------------------------------------------------------
void pqVRConnectionManager::add(pqVRPNConnection* conn)
{
  this->Internals->VRPNConnections.push_front(conn);
  conn->setQueue(this->Internals->Queue);
  emit this->connectionsChanged();
}

// ----------------------------------------------------------------------------
void pqVRConnectionManager::remove(pqVRPNConnection* conn)
{
  conn->stop();
  this->Internals->VRPNConnections.removeAll(conn);
  emit this->connectionsChanged();
}

// ----------------------------------------------------------------------------
pqVRPNConnection* pqVRConnectionManager::GetVRPNConnection(const QString& name)
{
  std::string target = name.toStdString();
  foreach (const QPointer<pqVRPNConnection>& conn, this->Internals->VRPNConnections)
  {
    if (!conn.isNull())
    {
      if (conn->name() == target)
      {
        return conn.data();
      }
    }
  }
  return NULL;
}
#endif

#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
// ----------------------------------------------------------------------------
void pqVRConnectionManager::add(pqVRUIConnection* conn)
{
  this->Internals->VRUIConnections.push_front(conn);
  conn->setQueue(this->Internals->Queue);
  emit this->connectionsChanged();
}

// ----------------------------------------------------------------------------
void pqVRConnectionManager::remove(pqVRUIConnection* conn)
{
  conn->stop();
  this->Internals->VRUIConnections.removeAll(conn);
  emit this->connectionsChanged();
}

// ----------------------------------------------------------------------------
pqVRUIConnection* pqVRConnectionManager::GetVRUIConnection(const QString& name)
{
  std::string target = name.toStdString();
  foreach (const QPointer<pqVRUIConnection>& conn, this->Internals->VRUIConnections)
  {
    if (!conn.isNull())
    {
      if (conn->name() == target)
      {
        return conn.data();
      }
    }
  }
  return NULL;
}
#endif

// ----------------------------------------------------------------------------
void pqVRConnectionManager::clear()
{
  this->stop();
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  this->Internals->VRPNConnections.clear();
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
  this->Internals->VRUIConnections.clear();
#endif
  emit this->connectionsChanged();
}

// ----------------------------------------------------------------------------
QList<QString> pqVRConnectionManager::connectionNames() const
{
  QList<QString> result;
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  foreach (pqVRPNConnection* conn, this->Internals->VRPNConnections)
  {
    if (conn)
    {
      result << QString::fromStdString(conn->name());
    }
  }
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
  foreach (pqVRUIConnection* conn, this->Internals->VRUIConnections)
  {
    if (conn)
    {
      result << QString::fromStdString(conn->name());
    }
  }
#endif
  qSort(result);
  return result;
}

// ----------------------------------------------------------------------------
int pqVRConnectionManager::numConnections() const
{
  int result = 0;
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  foreach (pqVRPNConnection* conn, this->Internals->VRPNConnections)
  {
    if (conn)
    {
      result++;
    }
  }
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
  foreach (pqVRUIConnection* conn, this->Internals->VRUIConnections)
  {
    if (conn)
    {
      result++;
    }
  }
#endif
  return result;
}

// ----------------------------------------------------------------------------
void pqVRConnectionManager::start()
{
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  foreach (pqVRPNConnection* conn, this->Internals->VRPNConnections)
  {
    if (conn && conn->init())
    {
      conn->start();
    }
  }
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
  foreach (pqVRUIConnection* conn, this->Internals->VRUIConnections)
  {
    if (conn && conn->init())
    {
      conn->start();
    }
  }
#endif
}

// ----------------------------------------------------------------------------
void pqVRConnectionManager::stop()
{
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  foreach (pqVRPNConnection* conn, this->Internals->VRPNConnections)
  {
    if (conn)
    {
      conn->stop();
    }
  }
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
  foreach (pqVRUIConnection* conn, this->Internals->VRUIConnections)
  {
    if (conn)
    {
      conn->stop();
    }
  }
#endif
}

// ----------------------------------------------------------------------------
void pqVRConnectionManager::configureConnections(vtkPVXMLElement* xml, vtkSMProxyLocator* locator)
{
  if (!xml)
  {
    return;
  }

  if (xml->GetName() && strcmp(xml->GetName(), "VRConnectionManager") == 0)
  {
    this->clear();
    for (unsigned cc = 0; cc < xml->GetNumberOfNestedElements(); cc++)
    {
      vtkPVXMLElement* child = xml->GetNestedElement(cc);
      if (child && child->GetName())
      {
        if (strcmp(child->GetName(), "VRPNConnection") == 0)
        {
          const char* name = child->GetAttributeOrEmpty("name");
          const char* address = child->GetAttributeOrEmpty("address");
// TODO: Need to throw some warning if VRPN is used when not
// compiled. For now we will simply ignore VRPN configuration
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
          pqVRPNConnection* device = new pqVRPNConnection(this);
          device->setName(name);
          device->setAddress(address);
          device->configure(child, locator);
          this->add(device);
#endif
        }
        else if (strcmp(child->GetName(), "VRUIConnection") == 0)
        {
// TODO: Need to throw some warning if VRUI is used when not
// compiled. For now we will simply ignore VRUI configuration
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
          const char* name = child->GetAttributeOrEmpty("name");
          const char* address = child->GetAttributeOrEmpty("address");
          const char* port = child->GetAttribute("port");
          pqVRUIConnection* device = new pqVRUIConnection(this);
          device->setName(name);
          device->setAddress(address);
          device->setPort(port ? port : "8555");
          device->configure(child, locator);
          this->add(device);
#endif
        }
        else
        {
          qWarning() << "Unknown Connection type : \"" << child->GetName() << "\"";
        }
      }
    }
  }
  else
  {
    this->configureConnections(xml->FindNestedElementByName("VRConnectionManager"), locator);
  }
  emit this->connectionsChanged();
}

// ----------------------------------------------------------------------------
void pqVRConnectionManager::saveConnectionsConfiguration(vtkPVXMLElement* root)
{
  assert(root != NULL);
  vtkPVXMLElement* tempParent = vtkPVXMLElement::New();
  tempParent->SetName("VRConnectionManager");
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRPN
  foreach (pqVRPNConnection* conn, this->Internals->VRPNConnections)
  {
    vtkPVXMLElement* child = conn->saveConfiguration();
    if (child)
    {
      tempParent->AddNestedElement(child);
      child->Delete();
    }
  }
#endif
#if PARAVIEW_PLUGIN_VRPlugin_USE_VRUI
  foreach (pqVRUIConnection* conn, this->Internals->VRUIConnections)
  {
    vtkPVXMLElement* child = conn->saveConfiguration();
    if (child)
    {
      tempParent->AddNestedElement(child);
      child->Delete();
    }
  }
#endif
  root->AddNestedElement(tempParent);
  tempParent->Delete();
}
