/*=========================================================================

   Program:   ParaQ
   Module:    pqPropertyLinks.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
#include <QPointer>
#include <QVariant>
#include <QSignalMapper>

// VTK includes
#include "vtkEventQtSlotConnect.h"

// ParaQ includes
#include "vtkSMProperty.h"
#include "pqSMAdaptor.h"


class pqPropertyLinksConnection::pqInternal
{
public:
  pqSMAdaptor::PropertyType Type;

  vtkSMProxy* Proxy;
  vtkSMProperty* Property;
  int Index;

  QPointer<QObject> QtObject;
  QByteArray QtProperty;
};

class pqPropertyLinks::pqInternal
{
public:
  pqInternal()
  {
    this->VTKConnections = vtkEventQtSlotConnect::New();
  }
  ~pqInternal()
  {
    this->VTKConnections->Delete();
  }

  // handle changes from the SM side
  vtkEventQtSlotConnect* VTKConnections;
  
  typedef vtkstd::multiset<pqPropertyLinksConnection> LinkMap;
  LinkMap Links;
};


pqPropertyLinksConnection::pqPropertyLinksConnection(vtkSMProxy* smproxy, vtkSMProperty* smproperty, int idx,
                                                     QObject* qobject, const char* qproperty)
{
  this->Internal = new pqPropertyLinksConnection::pqInternal;
  this->Internal->Proxy = smproxy;
  this->Internal->Property = smproperty;
  this->Internal->Index = idx;
  this->Internal->QtObject = qobject;
  this->Internal->QtProperty = qproperty;
  this->Internal->Type = pqSMAdaptor::getPropertyType(this->Internal->Property);
}

pqPropertyLinksConnection::pqPropertyLinksConnection(const pqPropertyLinksConnection& copy)
  : QObject()
{
  this->Internal = new pqPropertyLinksConnection::pqInternal;

  this->Internal->Type = copy.Internal->Type;

  this->Internal->Proxy = copy.Internal->Proxy;
  this->Internal->Property = copy.Internal->Property;
  this->Internal->Index = copy.Internal->Index;
  
  this->Internal->QtObject = copy.Internal->QtObject;
  this->Internal->QtProperty = copy.Internal->QtProperty;
}

pqPropertyLinksConnection::~pqPropertyLinksConnection()
{
  delete this->Internal;
}

pqPropertyLinksConnection& pqPropertyLinksConnection::operator=(const pqPropertyLinksConnection& copy)
{
  this->Internal->Type = copy.Internal->Type;

  this->Internal->Proxy = copy.Internal->Proxy;
  this->Internal->Property = copy.Internal->Property;
  this->Internal->Index = copy.Internal->Index;
  
  this->Internal->QtObject = copy.Internal->QtObject;
  this->Internal->QtProperty = copy.Internal->QtProperty;
  
  return *this;
}
bool pqPropertyLinksConnection::operator<(pqPropertyLinksConnection const& other) const
{
  if(this->Internal->Proxy < other.Internal->Proxy)
    {
    return true;
    }
  else if(this->Internal->Proxy > other.Internal->Proxy)
    {
    return false;
    }

  if(this->Internal->Property < other.Internal->Property)
    {
    return true;
    }
  else if(this->Internal->Property > other.Internal->Property)
    {
    return false;
    }
  
  if(this->Internal->Index < other.Internal->Index)
    {
    return true;
    }
  if(this->Internal->Index > other.Internal->Index)
    {
    return false;
    }
  
  if(this->Internal->QtObject < other.Internal->QtObject)
    {
    return true;
    }
  if(this->Internal->QtObject > other.Internal->QtObject)
    {
    return false;
    }
  
  return this->Internal->QtProperty < other.Internal->QtProperty;
}

void pqPropertyLinksConnection::smLinkedPropertyChanged() const
{
  if(this->Internal->QtObject)
    {
    // get the property of the object
    QVariant old = this->Internal->QtObject->property(this->Internal->QtProperty);
    QVariant prop;
    switch(this->Internal->Type)
      {
      case pqSMAdaptor::ENUMERATION:
        {
        prop = pqSMAdaptor::getEnumerationProperty(this->Internal->Proxy, this->Internal->Property);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty, prop);
          }
        }
      break;
      case pqSMAdaptor::SINGLE_ELEMENT:
        {
        prop = pqSMAdaptor::getElementProperty(this->Internal->Proxy, this->Internal->Property);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty, prop);
          }
        }
      break;
      case pqSMAdaptor::MULTIPLE_ELEMENTS:
        {
        prop = pqSMAdaptor::getMultipleElementProperty(this->Internal->Proxy, this->Internal->Property, this->Internal->Index);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty, prop);
          }
        }
      break;
      default:
        {
        }
      }
    }
}

void pqPropertyLinksConnection::qtLinkedPropertyChanged() const
{
  if(this->Internal->QtObject)
    {
    // get the property of the object
    QVariant prop = this->Internal->QtObject->property(this->Internal->QtProperty);
    QVariant old;
    switch(this->Internal->Type)
      {
      case pqSMAdaptor::ENUMERATION:
        {
        old = pqSMAdaptor::getEnumerationProperty(this->Internal->Proxy, this->Internal->Property);
        if(prop != old)
          {
          pqSMAdaptor::setEnumerationProperty(this->Internal->Proxy, this->Internal->Property, prop);
          }
        }
      break;
      case pqSMAdaptor::SINGLE_ELEMENT:
        {
        old = pqSMAdaptor::getElementProperty(this->Internal->Proxy, this->Internal->Property);
        if(prop != old)
          {
          pqSMAdaptor::setElementProperty(this->Internal->Proxy, this->Internal->Property, prop);
          }
        }
      break;
      case pqSMAdaptor::MULTIPLE_ELEMENTS:
        {
        old = pqSMAdaptor::getMultipleElementProperty(this->Internal->Proxy, this->Internal->Property, this->Internal->Index);
        if(prop != old)
          {
          pqSMAdaptor::setMultipleElementProperty(this->Internal->Proxy, this->Internal->Property, this->Internal->Index, prop);
          }
        }
      break;
      default:
        {
        }
      }
    }
}

class pqPropertyLinksInternal
{
public:
  pqPropertyLinksInternal()
    {
    this->VTKConnections = vtkEventQtSlotConnect::New();
    }
  ~pqPropertyLinksInternal()
    {
    this->VTKConnections->Delete();
    }

  // handle changes from the SM side
  vtkEventQtSlotConnect* VTKConnections;
  typedef vtkstd::multiset<pqPropertyLinksConnection> LinkMap;
  LinkMap Links;
};

pqPropertyLinks::pqPropertyLinks(QObject* p)
  : QObject(p)
{
  this->Internal = new pqPropertyLinks::pqInternal;
}

pqPropertyLinks::~pqPropertyLinks()
{
  delete this->Internal;
}

void pqPropertyLinks::addPropertyLink(QObject* qObject, const char* qProperty, const char* signal,
                     vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  if(!Proxy || !Property || !qObject || !qProperty || !signal)
    {
    return;
    }

  pqPropertyLinksInternal::LinkMap::iterator iter;
  pqPropertyLinksConnection conn(Proxy, Property, Index, qObject, qProperty);
  iter = this->Internal->Links.insert(conn);
  this->Internal->VTKConnections->Connect(Property, vtkCommand::ModifiedEvent,
                                          &*iter, 
                                          SLOT(smLinkedPropertyChanged()));
  
  QObject::connect(qObject, signal, &*iter, SLOT(qtLinkedPropertyChanged()));
  
  // set the object property to the current server manager property value
  iter->smLinkedPropertyChanged();
}

void pqPropertyLinks::removePropertyLink(QObject* qObject, const char* qProperty, const char* signal,
                        vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  pqPropertyLinksConnection conn(Proxy, Property, Index, qObject, qProperty);
  pqPropertyLinksInternal::LinkMap::iterator iter;
  iter = this->Internal->Links.find(conn);
  if(iter != this->Internal->Links.end())
    {
    this->Internal->VTKConnections->Disconnect(iter->Internal->Property, vtkCommand::ModifiedEvent, qObject);
    QObject::disconnect(iter->Internal->QtObject, signal, &*iter, SLOT(qtLinkedPropertyChanged()));
    }
}

