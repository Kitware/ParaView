/*=========================================================================

   Program: ParaView
   Module:    pqPropertyLinks.cxx

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

// self include
#include "pqPropertyLinks.h"

// standard includes
#include <vtkstd/set>

// Qt includes
#include <QString>
#include <QStringList>
#include <QPointer>
#include <QVariant>
#include <QSet>
#include <QSignalMapper>
#include <QTimer>
#include <QtDebug>

// VTK includes
#include "vtkEventQtSlotConnect.h"

// ParaView includes
#include "vtkSMProperty.h"
#include "pqSMAdaptor.h"


class pqPropertyLinksConnection::pqInternal
{
public:
  pqInternal()
  {
    this->UseUncheckedProperties = false;
    this->AutoUpdate = true;
    this->Updating = false;
  }

  ~pqInternal()
    {
    }

  vtkSMProxy* Proxy;
  vtkSMProperty* Property;
  int Index;

  QPointer<QObject> QtObject;
  QByteArray QtProperty;
  bool UseUncheckedProperties;
  bool AutoUpdate;
  bool Updating;

  // This flag indicates if the QObject and the vtkSMProperty are out of synch. 
  bool OutOfSync;
};

class pqPropertyLinks::pqInternal
{
public:
  pqInternal()
  {
    this->VTKConnections = vtkEventQtSlotConnect::New();
    this->UseUncheckedProperties = false;
    this->AutoUpdate = true;
  }
  ~pqInternal()
  {
    this->VTKConnections->Delete();
  }

  // handle changes from the SM side
  vtkEventQtSlotConnect* VTKConnections;
  
  typedef QList<QPointer<pqPropertyLinksConnection> > 
    ListOfPropertyLinkConnections;
  ListOfPropertyLinkConnections Links;
  bool UseUncheckedProperties;
  bool AutoUpdate;
};


pqPropertyLinksConnection::pqPropertyLinksConnection(
  QObject* _parent, vtkSMProxy* smproxy, 
  vtkSMProperty* smproperty, int idx, QObject* qobject, const char* qproperty)
: QObject(_parent)
{
  this->Internal = new pqPropertyLinksConnection::pqInternal;
  this->Internal->Proxy = smproxy;
  this->Internal->Property = smproperty;
  this->Internal->Index = idx;
  this->Internal->QtObject = qobject;
  this->Internal->QtProperty = qproperty;
  this->Internal->OutOfSync = false;
}

pqPropertyLinksConnection::~pqPropertyLinksConnection()
{
  delete this->Internal;
}

bool pqPropertyLinksConnection::isEqual(vtkSMProxy* proxy, vtkSMProperty* smproperty, int idx,
    QObject* qObject, const char* qproperty) const
{
  return (this->Internal->Proxy == proxy &&
    this->Internal->Property == smproperty &&
    this->Internal->Index == idx &&
    this->Internal->QtObject == qObject &&
    this->Internal->QtProperty == qproperty);
}

bool pqPropertyLinksConnection::getOutOfSync() const
{
  return this->Internal->OutOfSync;
}

void pqPropertyLinksConnection::clearOutOfSync() const
{
  this->Internal->OutOfSync = false;
}

void pqPropertyLinksConnection::triggerDelayedSMLinkedPropertyChanged()
{
  if(this->Internal->Updating == false)
    {
    QTimer::singleShot(0, this, SLOT(smLinkedPropertyChanged()));
    this->Internal->Updating = true;
    }
}

void pqPropertyLinksConnection::smLinkedPropertyChanged() 
{
  this->Internal->Updating = false;
  this->Internal->OutOfSync = true;

  if(this->Internal->QtObject)
    {
    // get the property of the object
    QVariant old;
    old = this->Internal->QtObject->property(this->Internal->QtProperty);
    QVariant prop;
    switch(pqSMAdaptor::getPropertyType(this->Internal->Property))
      {
    case pqSMAdaptor::PROXY:
    case pqSMAdaptor::PROXYSELECTION:
        {
        pqSMProxy p;
        p = pqSMAdaptor::getProxyProperty(this->Internal->Property);
        prop.setValue(p);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
            prop);
          }
        }
      break;
    case pqSMAdaptor::ENUMERATION:
        {
        prop = pqSMAdaptor::getEnumerationProperty(this->Internal->Property);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
            prop);
          }
        }
      break;
    case pqSMAdaptor::SINGLE_ELEMENT:
        {
        prop = pqSMAdaptor::getElementProperty(this->Internal->Property);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
            prop);
          }
        }
      break;
    case pqSMAdaptor::FILE_LIST:
        {
        prop = pqSMAdaptor::getFileListProperty(this->Internal->Property);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty,
            prop);
          }
        }
      break;
    case pqSMAdaptor::SELECTION:
        {
        if(this->Internal->Index == -1)
          {
          QList<QList<QVariant> > newVal =
            pqSMAdaptor::getSelectionProperty(this->Internal->Property);
          if(newVal != old.value<QList<QList<QVariant> > >())
            {
            prop.setValue(newVal);
            this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
              prop);
            }
          }
        else
          {
          QList<QVariant> sel;
          sel = pqSMAdaptor::getSelectionProperty(this->Internal->Property, 
            this->Internal->Index);

          if(sel.size() == 2 && sel[1] != old)
            {
            this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
              sel[1]);
            }
          }
        }
      break;
    case pqSMAdaptor::MULTIPLE_ELEMENTS:
    case pqSMAdaptor::COMPOSITE_TREE:
    case pqSMAdaptor::SIL:
        {
        if(this->Internal->Index == -1)
          {
          prop = pqSMAdaptor::getMultipleElementProperty(
            this->Internal->Property);
          if(prop != old)
            {
            this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
              prop);
            }
          }
        else
          {
          prop = pqSMAdaptor::getMultipleElementProperty(
            this->Internal->Property, 
            this->Internal->Index);
          if(prop != old)
            {
            this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
              prop);
            }
          }
        }
      break;
    case pqSMAdaptor::FIELD_SELECTION:
        {
        if(this->Internal->Index == 0)
          {
          prop = pqSMAdaptor::getFieldSelectionMode(
            this->Internal->Property);
          if(prop != old)
            {
            this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
              prop);
            }
          }
        else
          {
          prop = pqSMAdaptor::getFieldSelectionScalar(
            this->Internal->Property);
          if(prop != old)
            {
            this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
              prop);
            }
          }
        }
    case pqSMAdaptor::UNKNOWN:
    case pqSMAdaptor::PROXYLIST:
      break;
      }
    }
  emit this->smPropertyChanged();
}

void pqPropertyLinksConnection::qtLinkedPropertyChanged() 
{
  this->Internal->OutOfSync = true;

  if(this->Internal->QtObject)
    {
    // get the property of the object
    QVariant prop;
    prop = this->Internal->QtObject->property(this->Internal->QtProperty);
    switch(pqSMAdaptor::getPropertyType(this->Internal->Property))
      {
    case pqSMAdaptor::PROXY:
    case pqSMAdaptor::PROXYSELECTION:
        {
        if(this->Internal->UseUncheckedProperties)
          {
          pqSMAdaptor::setUncheckedProxyProperty(this->Internal->Property,
            prop.value<pqSMProxy>());
          }
        else
          {
          pqSMAdaptor::setProxyProperty(this->Internal->Property,
            prop.value<pqSMProxy>());
          if(this->Internal->AutoUpdate)
            {
            this->Internal->Proxy->UpdateVTKObjects();
            }
          }
        }
      break;
    case pqSMAdaptor::ENUMERATION:
        {
        if(this->Internal->UseUncheckedProperties)
          {
          pqSMAdaptor::setUncheckedEnumerationProperty(
            this->Internal->Property, prop);
          }
        else
          {
          pqSMAdaptor::setEnumerationProperty(
            this->Internal->Property, prop);
          if(this->Internal->AutoUpdate)
            {
            this->Internal->Proxy->UpdateVTKObjects();
            }
          }
        }
      break;
    case pqSMAdaptor::SINGLE_ELEMENT:
        {
        if(this->Internal->UseUncheckedProperties)
          {
          pqSMAdaptor::setUncheckedElementProperty(
            this->Internal->Property, prop);
          }
        else
          {
          pqSMAdaptor::setElementProperty(
            this->Internal->Property, prop);
          if(this->Internal->AutoUpdate)
            {
            this->Internal->Proxy->UpdateVTKObjects();
            }
          }
        }
      break;
    case pqSMAdaptor::FILE_LIST:
        {
        if(this->Internal->UseUncheckedProperties)
          {
          if (!prop.canConvert<QStringList>())
            {
            qWarning() << "File list is not a list.";
            }
          else
            {
            pqSMAdaptor::setUncheckedFileListProperty(
                           this->Internal->Property, prop.value<QStringList>());
            }
          }
        else
          {
          if (!prop.canConvert<QStringList>())
            {
            qWarning() << "File list is not a list.";
            }
          else
            {
            pqSMAdaptor::setFileListProperty(
                           this->Internal->Property, prop.value<QStringList>());
            }
          if(this->Internal->AutoUpdate)
            {
            this->Internal->Proxy->UpdateVTKObjects();
            }
          }
        }
      break;
    case pqSMAdaptor::SELECTION:
        {
        if(this->Internal->Index == -1)
          {
          QList<QList<QVariant> > theProp = prop.value<QList<QList<QVariant> > >();
          if(this->Internal->UseUncheckedProperties)
            {
            pqSMAdaptor::setUncheckedSelectionProperty(
              this->Internal->Property, theProp);
            }
          else
            {
            pqSMAdaptor::setSelectionProperty(
              this->Internal->Property, theProp);
            if(this->Internal->AutoUpdate)
              {
              this->Internal->Proxy->UpdateVTKObjects();
              }
            }
          }
        else
          {
          QList<QVariant> domain;
          domain = pqSMAdaptor::getSelectionPropertyDomain(
            this->Internal->Property);
          QList<QVariant> selection;
          selection.append(domain[this->Internal->Index]);
          selection.append(prop);

          if(this->Internal->UseUncheckedProperties)
            {
            pqSMAdaptor::setUncheckedSelectionProperty(
              this->Internal->Property, selection);
            }
          else
            {
            pqSMAdaptor::setSelectionProperty(
              this->Internal->Property, selection);
            if(this->Internal->AutoUpdate)
              {
              this->Internal->Proxy->UpdateVTKObjects();
              }
            }
          }
        }
      break;
    case pqSMAdaptor::SIL:
    case pqSMAdaptor::MULTIPLE_ELEMENTS:
    case pqSMAdaptor::COMPOSITE_TREE:
        {
        if(this->Internal->Index == -1)
          {
          if(this->Internal->UseUncheckedProperties)
            {
            pqSMAdaptor::setUncheckedMultipleElementProperty(
              this->Internal->Property, prop.toList());
            }
          else
            {
            pqSMAdaptor::setMultipleElementProperty(
              this->Internal->Property, prop.toList());
            if(this->Internal->AutoUpdate)
              {
              this->Internal->Proxy->UpdateVTKObjects();
              }
            }
          }
        else
          {
          if(this->Internal->UseUncheckedProperties)
            {
            pqSMAdaptor::setUncheckedMultipleElementProperty(
              this->Internal->Property, this->Internal->Index, prop);
            }
          else
            {
            pqSMAdaptor::setMultipleElementProperty(
              this->Internal->Property, this->Internal->Index, prop);
            if(this->Internal->AutoUpdate)
              {
              this->Internal->Proxy->UpdateVTKObjects();
              }
            }
          }
        }
      break;
    case pqSMAdaptor::FIELD_SELECTION:
        {
        if(this->Internal->Index == 0)
          {
          if(this->Internal->UseUncheckedProperties)
            {
            pqSMAdaptor::setUncheckedFieldSelectionMode(
              this->Internal->Property, prop.toString());
            }
          else
            {
            pqSMAdaptor::setFieldSelectionMode(
              this->Internal->Property, prop.toString());
            if(this->Internal->AutoUpdate)
              {
              this->Internal->Proxy->UpdateVTKObjects();
              }
            }
          }
        else
          {
          if(this->Internal->UseUncheckedProperties)
            {
            pqSMAdaptor::setUncheckedFieldSelectionScalar(
              this->Internal->Property, prop.toString());
            }
          else
            {
            pqSMAdaptor::setFieldSelectionScalar(
              this->Internal->Property, prop.toString());
            if(this->Internal->AutoUpdate)
              {
              this->Internal->Proxy->UpdateVTKObjects();
              }
            }
          }
        }
      break;
    case pqSMAdaptor::UNKNOWN:
    case pqSMAdaptor::PROXYLIST:
      break;
      }
    }
  emit this->qtWidgetChanged();
}

bool pqPropertyLinksConnection::useUncheckedProperties() const
{
  return this->Internal->UseUncheckedProperties;
}

void pqPropertyLinksConnection::setUseUncheckedProperties(bool flag) const
{
  this->Internal->UseUncheckedProperties = flag;
}

void pqPropertyLinksConnection::setAutoUpdateVTKObjects(bool flag) const
{
  this->Internal->AutoUpdate = flag;
}

bool pqPropertyLinksConnection::autoUpdateVTKObjects() const
{
  return this->Internal->AutoUpdate;
}


pqPropertyLinks::pqPropertyLinks(QObject* p)
  : QObject(p)
{
  this->Internal = new pqPropertyLinks::pqInternal;
}

pqPropertyLinks::~pqPropertyLinks()
{
  delete this->Internal;
}

void pqPropertyLinks::addPropertyLink(QObject* qObject, const char* qProperty, 
                                      const char* signal,
                                      vtkSMProxy* Proxy, 
                                      vtkSMProperty* Property, int Index)
{
  if(!Proxy || !Property || !qObject || !qProperty || !signal)
    {
    qWarning("Invalid parameters to add link\n");
    qDebug() << "Proxy:" << Proxy << Proxy->GetClassName();
    qDebug() << "Property:" << Property;
    qDebug() << "qObject:" << qObject;
    qDebug() << "qProperty:" << qProperty;
    qDebug() << "signal:" << signal;
    return;
    }
  
  pqPropertyLinksConnection *conn = 
    new pqPropertyLinksConnection(this, Proxy, Property, Index, qObject, qProperty);
  this->Internal->Links.push_back(conn);
  this->Internal->VTKConnections->Connect(Property, vtkCommand::ModifiedEvent,
                                          conn, 
                                          SLOT(triggerDelayedSMLinkedPropertyChanged()));
  
  QObject::connect(qObject, signal,conn, SLOT(qtLinkedPropertyChanged()));

  QObject::connect(conn, SIGNAL(qtWidgetChanged()), 
    this, SIGNAL(qtWidgetChanged()));
  QObject::connect(conn, SIGNAL(smPropertyChanged()),
    this, SIGNAL(smPropertyChanged()));
  
  conn->setUseUncheckedProperties(this->Internal->UseUncheckedProperties);
  conn->setAutoUpdateVTKObjects(this->Internal->AutoUpdate);
  // set the object property to the current server manager property value
  conn->smLinkedPropertyChanged();
  // We let the connection be marked dirty on creation.
  // conn->clearOutOfSync();
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::removePropertyLink(QObject* qObject, 
                        const char* qProperty, const char* signal,
                        vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  foreach (pqPropertyLinksConnection* conn, this->Internal->Links)
    {
    if (conn && conn->isEqual(Proxy, Property, Index, qObject, qProperty))
      {
      this->Internal->VTKConnections->Disconnect(conn->Internal->Property, 
        vtkCommand::ModifiedEvent, conn);
      QObject::disconnect(conn->Internal->QtObject, signal, conn, 
        SLOT(qtLinkedPropertyChanged()));
      QObject::disconnect(conn, 0, this, 0);
      delete conn;
      break;
      }
    }
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::removeAllPropertyLinks()
{
  foreach (pqPropertyLinksConnection* conn, this->Internal->Links)
    {
    if (conn)
      {
      this->Internal->VTKConnections->Disconnect(conn->Internal->Property, 
        vtkCommand::ModifiedEvent, conn);
      QObject::disconnect(conn->Internal->QtObject, 0, conn, 0);
      QObject::disconnect(conn, 0, this, 0);
      }
    delete conn;
    }
  this->Internal->Links.clear();
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::reset()
{
  foreach(pqPropertyLinksConnection* conn, this->Internal->Links)
    {
    if (conn && conn->getOutOfSync())
      {
      conn->smLinkedPropertyChanged();
      conn->clearOutOfSync();
      }
    }
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::accept()
{
  bool old = this->useUncheckedProperties();
  bool oldauto = this->autoUpdateVTKObjects();

  QSet<vtkSMProxy*> ChangedProxies;

  foreach(pqPropertyLinksConnection* conn, this->Internal->Links)
    {
    if (!conn || !conn->getOutOfSync())
      {
      continue;
      }
    conn->setUseUncheckedProperties(false);
    conn->setAutoUpdateVTKObjects(false);
    conn->qtLinkedPropertyChanged();
    conn->setAutoUpdateVTKObjects(oldauto);
    conn->setUseUncheckedProperties(old);
    conn->clearOutOfSync();

    ChangedProxies.insert(conn->Internal->Proxy);
    }

  foreach(vtkSMProxy* p, ChangedProxies)
    {
    p->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
bool pqPropertyLinks::useUncheckedProperties()
{
  return this->Internal->UseUncheckedProperties;
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::setUseUncheckedProperties(bool flag)
{
  this->Internal->UseUncheckedProperties = flag;
  
  foreach(pqPropertyLinksConnection* conn, this->Internal->Links)
    {
    conn->setUseUncheckedProperties(flag);
    }
}

//-----------------------------------------------------------------------------
bool pqPropertyLinks::autoUpdateVTKObjects()
{
  return this->Internal->AutoUpdate;
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::setAutoUpdateVTKObjects(bool flag)
{
  this->Internal->AutoUpdate = flag;
  
  foreach(pqPropertyLinksConnection* conn, this->Internal->Links)
    {
    conn->setAutoUpdateVTKObjects(flag);
    }
}

