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
#include "vtkVRConnectionManager.h"

// --------------------------------------------------------------------includes
#include "pqApplicationCore.h"
#include <QPointer>
#include "vtkPVVRConfig.h"
#ifdef PARAVIEW_USE_VRPN
#include "vtkVRPNConnection.h"
#endif
#ifdef PARAVIEW_USE_VRUI
#include "vtkVRUIConnection.h"
#endif
#include "vtkPVXMLElement.h"
#include "vtkVRQueue.h"
#include "vtkObjectFactory.h"
#include <QList>
#include <QtDebug>

// -----------------------------------------------------------------------macro

// --------------------------------------------------------------------internal
// IMPORTANT: Make sure that this struct has no pointers.  All pointers should
// be put in the class declaration. For all newly defined pointers make sure to
// update constructor and destructor methods.
struct vtkVRConnectionManager::pqInternals
{
#ifdef PARAVIEW_USE_VRPN
  QList<QPointer<vtkVRPNConnection> > VRPNConnections;
#endif
#ifdef PARAVIEW_USE_VRUI
  QList<QPointer<vtkVRUIConnection> > VRUIConnections;
#endif
  QPointer<vtkVRQueue> Queue;
};

// -----------------------------------------------------------------------cnstr
vtkVRConnectionManager::vtkVRConnectionManager(vtkVRQueue* queue,
                                               QObject* parentObject ):
  Superclass( parentObject )
{
  this->Internals = new pqInternals();
  this->Internals->Queue = queue;
  QObject::connect(pqApplicationCore::instance(),
    SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)),
    this, SLOT(configureConnections(vtkPVXMLElement*, vtkSMProxyLocator*)));
  QObject::connect(pqApplicationCore::instance(),
    SIGNAL(stateSaved(vtkPVXMLElement*)),
    this, SLOT(saveConnectionsConfiguration(vtkPVXMLElement*)));
}

// -----------------------------------------------------------------------destr
vtkVRConnectionManager::~vtkVRConnectionManager()
{
  delete this->Internals;
}

#ifdef PARAVIEW_USE_VRPN
void vtkVRConnectionManager::add( vtkVRPNConnection* conn )
{
  this->Internals->VRPNConnections.push_front( conn );
}

void vtkVRConnectionManager::remove( vtkVRPNConnection *conn )
{
  conn->Stop();
  this->Internals->VRPNConnections.removeAll( conn );
}
#endif
#ifdef PARAVIEW_USE_VRUI
void vtkVRConnectionManager::add( vtkVRUIConnection* conn )
{
  this->Internals->VRUIConnections.push_front( conn );
}

void vtkVRConnectionManager::remove( vtkVRUIConnection *conn )
{
  conn->Stop();
  this->Internals->VRUIConnections.removeAll( conn );
}
#endif
void vtkVRConnectionManager::clear()
{
  this->stop();
#ifdef PARAVIEW_USE_VRPN
  this->Internals->VRPNConnections.clear();
#endif
#ifdef PARAVIEW_USE_VRUI
  this->Internals->VRUIConnections.clear();
#endif
}

void vtkVRConnectionManager::start()
{
#ifdef PARAVIEW_USE_VRPN
  foreach (vtkVRPNConnection* conn, this->Internals->VRPNConnections )
    {
    if (conn && conn->Init())
      {
        conn->start();
      }
    }
#endif
#ifdef PARAVIEW_USE_VRUI
  foreach (vtkVRUIConnection* conn, this->Internals->VRUIConnections )
    {
    if (conn && conn->Init())
      {
        conn->start();
      }
    }
#endif
}

void vtkVRConnectionManager::stop()
{
#ifdef PARAVIEW_USE_VRPN
  foreach (vtkVRPNConnection* conn, this->Internals->VRPNConnections )
    {
    if (conn)
      {
        conn->Stop();
      }
    }
#endif
#ifdef PARAVIEW_USE_VRUI
    foreach (vtkVRUIConnection* conn, this->Internals->VRUIConnections )
    {
    if (conn)
      {
        conn->Stop();
      }
    }
#endif
}

void vtkVRConnectionManager::configureConnections( vtkPVXMLElement* xml,
                                                  vtkSMProxyLocator* locator )
{
  if (!xml)
    {
    return;
    }

    if (xml->GetName() && strcmp(xml->GetName(), "VRConnectionManager") == 0)
      {
      this->clear();
      for (unsigned cc=0; cc < xml->GetNumberOfNestedElements(); cc++)
        {
        vtkPVXMLElement* child = xml->GetNestedElement(cc);
        if (child && child->GetName() )
          {
          if (strcmp(child->GetName(), "VRPNConnection")==0)
            {
            const char* name = child->GetAttributeOrEmpty( "name" );
            const char* address = child->GetAttributeOrEmpty( "address" );
#ifdef PARAVIEW_USE_VRPN        // TODO: Need to throw some warning if VRPN is
                                // used when not compiled. For now we will
                                // simply ignore VRPN configuration
            vtkVRPNConnection* device = new vtkVRPNConnection(this);
            device->SetQueue( this->Internals->Queue );
            device->SetName( name );
            device->SetAddress( address );
            device->configure(child, locator);
            this->add(device);
#endif
            }
          else if (strcmp(child->GetName(), "VRUIConnection")==0)
            {
            const char* name = child->GetAttributeOrEmpty( "name" );
            const char* address = child->GetAttributeOrEmpty( "address" );
            const char* port = child->GetAttribute( "port" );
#ifdef PARAVIEW_USE_VRUI        // TODO: Need to throw some warning if VRUI is
                                // used when not compiled. For not we will
                                // simply ignore VRUI configuration
            vtkVRUIConnection* device = new vtkVRUIConnection(this);
            device->SetQueue( this->Internals->Queue );
            device->SetName( name );
            device->SetAddress( address );
            ( port )
              ? device->SetPort( port )
              : device->SetPort("8555"); // default
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
      this->start();
      }
    else
      {
      this->configureConnections(xml->FindNestedElementByName("VRConnectionManager"),
                                 locator);
      }
}

void vtkVRConnectionManager::saveConnectionsConfiguration( vtkPVXMLElement* root )
{
 Q_ASSERT(root != NULL);
  vtkPVXMLElement* tempParent = vtkPVXMLElement::New();
  tempParent->SetName("VRConnectionManager");
#ifdef PARAVIEW_USE_VRPN
  foreach (vtkVRPNConnection* conn, this->Internals->VRPNConnections )
    {
    vtkPVXMLElement* child = conn->saveConfiguration();
    if (child)
      {
      tempParent->AddNestedElement(child);
      child->Delete();
      }
    }
#endif
#ifdef PARAVIEW_USE_VRUI
  foreach (vtkVRUIConnection* conn, this->Internals->VRUIConnections )
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
