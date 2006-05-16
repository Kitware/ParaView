/*=========================================================================

   Program:   ParaQ
   Module:    pqPropertyManager.cxx

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
#include "pqPropertyManager.h"

// Qt includes
#include <QByteArray>
#include <QMultiMap>
#include <QApplication>

// ParaQ includes
#include "pqPropertyLinks.h"

class pqPropertyManager::pqInternal
{
public:
  struct PropertyKey
    {
    PropertyKey(vtkSMProperty* p, int i) : Property(p), Index(i) {}
    vtkSMProperty* Property;
    int Index;
    bool operator<(const PropertyKey& o) const
      {
      if(this->Property < o.Property)
        {
        return true;
        }
      if(this->Property > o.Property)
        {
        return false;
        }
      return this->Index < o.Index;
      }
    };
  typedef QMultiMap<PropertyKey, pqPropertyManagerProperty*> PropertyMap;
  PropertyMap Properties;
  pqPropertyLinks Links;
};

pqPropertyManagerProperty::pqPropertyManagerProperty(QObject* p)
  : QObject(p)
{
}

pqPropertyManagerProperty::~pqPropertyManagerProperty()
{
  QList<pqPropertyManagerPropertyLink*>::iterator iter;
  for(iter = this->Links.begin(); iter != this->Links.end(); ++iter)
    {
    delete (*iter);
    }
}

QVariant pqPropertyManagerProperty::value() const
{
  return this->Value;
}

void pqPropertyManagerProperty::setValue(const QVariant& v)
{
  if(this->Value != v)
    {
    this->Value = v;
    emit this->propertyChanged();
    }
}

void pqPropertyManagerProperty::addLink(QObject* o, const char* prop, const char* signal)
{
  pqPropertyManagerPropertyLink* link = new pqPropertyManagerPropertyLink(this, o, prop, signal);
  this->Links.append(link);
  o->setProperty(prop, this->Value);
}

void pqPropertyManagerProperty::removeLink(QObject* o, const char* prop, const char* /*signal*/)
{
  QList<pqPropertyManagerPropertyLink*>::iterator iter;
  for(iter = this->Links.begin(); iter != this->Links.end(); ++iter)
    {
    if((*iter)->object() == o && (*iter)->property() == prop)
      {
      delete (*iter);
      this->Links.erase(iter);
      return;
      }
    }
}

pqPropertyManagerPropertyLink::pqPropertyManagerPropertyLink(pqPropertyManagerProperty* p, 
                                     QObject* o, const char* prop, const char* signal)
 : QObject(p), QtObject(o), QtProperty(prop)
{
  QObject::connect(p, SIGNAL(propertyChanged()), this, SLOT(managerPropertyChanged()));
  QObject::connect(o, signal, this, SLOT(guiPropertyChanged()));
}

void pqPropertyManagerPropertyLink::guiPropertyChanged()
{
  pqPropertyManagerProperty* p = qobject_cast<pqPropertyManagerProperty*>(this->parent());
  QVariant v = this->QtObject->property(QtProperty);
  if(p->value() != v)
    {
    p->setValue(v);
    }
}

void pqPropertyManagerPropertyLink::managerPropertyChanged()
{
  pqPropertyManagerProperty* p = qobject_cast<pqPropertyManagerProperty*>(this->parent());
  QVariant v = p->value();
  if(this->QtObject && this->QtObject->property(QtProperty) != v)
    {
    this->QtObject->setProperty(QtProperty, v);
    }
}

QObject* pqPropertyManagerPropertyLink::object() const
{
  return this->QtObject;
}

QByteArray pqPropertyManagerPropertyLink::property() const
{
  return this->QtProperty;
}


pqPropertyManager::pqPropertyManager(QObject* p)
  : QObject(p)
{
  this->Internal = new pqPropertyManager::pqInternal;
}

pqPropertyManager::~pqPropertyManager()
{
  foreach(pqPropertyManagerProperty* l, this->Internal->Properties.values())
    {
    delete l;
    }
  delete this->Internal;
}

void pqPropertyManager::registerLink(QObject* qObject, const char* qProperty, const char* signal,
                     vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  pqInternal::PropertyMap::iterator iter;
  iter = this->Internal->Properties.find(pqInternal::PropertyKey(Property, Index));
  if(iter == this->Internal->Properties.end())
    {
    pqPropertyManagerProperty* p = new pqPropertyManagerProperty(NULL);
    iter = this->Internal->Properties.insert(pqInternal::PropertyKey(Property, Index), p);
    
    this->Internal->Links.addPropertyLink(iter.value(), "value", SIGNAL(flushProperty()),
      Proxy, Property, Index);

    QObject::connect(p, SIGNAL(propertyChanged()), this, SLOT(propertyChanged()));
    }
  // link the QObject property with this QObject property
  iter.value()->addLink(qObject, qProperty, signal);
}


void pqPropertyManager::unregisterLink(QObject* qObject, const char* qProperty, const char* signal,
                        vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  pqInternal::PropertyMap::iterator iter;
  iter = this->Internal->Properties.find(pqInternal::PropertyKey(Property, Index));
  if(iter != this->Internal->Properties.end())
    {
    iter.value()->removeLink(qObject, qProperty, signal);
    if(iter.value()->numberOfLinks() == 0)
      {
      QObject::disconnect(iter.value(), SIGNAL(propertyChanged()), this, SLOT(propertyChanged()));
      this->Internal->Links.removePropertyLink(iter.value(), "value", SIGNAL(flushProperty()),
        Proxy, Property, Index);
      this->Internal->Properties.erase(iter);
      }
    }
}


void pqPropertyManager::accept()
{
  emit this->preaccept();
  foreach(pqPropertyManagerProperty* p, this->Internal->Properties.values())
    {
    emit p->flushProperty();
    }
  // hack -- apparently our propertyChanged() slot is called after we emit canAcceptOrReject(),
  // so flushing the queue works around that
  QApplication::processEvents();  
  emit this->canAcceptOrReject(false);
  emit this->accepted();
  emit this->postaccept();
}

void pqPropertyManager::reject()
{
  emit this->prereject();
  this->Internal->Links.refreshLinks();
  // hack -- apparently our propertyChanged() slot is called after we emit canAcceptOrReject(),
  // so flushing the queue works around that
  QApplication::processEvents();
  emit this->canAcceptOrReject(false);
  emit postreject();
}

void pqPropertyManager::propertyChanged()
{
  emit this->canAcceptOrReject(true);
}

