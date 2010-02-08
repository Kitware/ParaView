/*=========================================================================

   Program: ParaView
   Module:    pqPropertyManager.cxx

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
#include "pqPropertyManager.h"

// Qt includes
#include <QByteArray>
#include <QMultiMap>
#include <QApplication>

// ParaView includes
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
  pqInternal()
    {
    Links.setUseUncheckedProperties(true);
    Modified = false;
    }
  typedef QMultiMap<PropertyKey, pqPropertyManagerProperty*> PropertyMap;
  PropertyMap Properties;
  pqPropertyLinks Links;
  bool Modified;
};

//-----------------------------------------------------------------------------
pqPropertyManagerProperty::pqPropertyManagerProperty(QObject* p)
  : QObject(p)
{
}

//-----------------------------------------------------------------------------
pqPropertyManagerProperty::~pqPropertyManagerProperty()
{
  QList<pqPropertyManagerPropertyLink*>::iterator iter;
  for(iter = this->Links.begin(); iter != this->Links.end(); ++iter)
    {
    delete (*iter);
    }
}

//-----------------------------------------------------------------------------
QVariant pqPropertyManagerProperty::value() const
{
  return this->Value;
}

//-----------------------------------------------------------------------------
void pqPropertyManagerProperty::setValue(const QVariant& v)
{
  if(this->Value != v)
    {
    this->Value = v;
    emit this->propertyChanged();
    }
}

//-----------------------------------------------------------------------------
void pqPropertyManagerProperty::addLink(
  QObject* o, const char* prop, const char* signal)
{
  pqPropertyManagerPropertyLink* link = 
    new pqPropertyManagerPropertyLink(this, o, prop, signal);
  this->Links.append(link);
  o->setProperty(prop, this->Value);
}

//-----------------------------------------------------------------------------
void pqPropertyManagerProperty::removeLink(
  QObject* o, const char* prop, const char* /*signal*/)
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

//-----------------------------------------------------------------------------
void pqPropertyManagerProperty::removeAllLinks()
{
  QList<pqPropertyManagerPropertyLink*>::iterator iter;
  for (iter = this->Links.begin(); iter != this->Links.end(); ++iter)
    {
    delete (*iter);
    }
  this->Links.clear();
}

//-----------------------------------------------------------------------------
pqPropertyManagerPropertyLink::pqPropertyManagerPropertyLink(
  pqPropertyManagerProperty* p, 
  QObject* o, const char* prop, const char* signal)
  : QObject(p), QtObject(o), QtProperty(prop)
{
  QObject::connect(p, SIGNAL(propertyChanged()), 
                   this, SLOT(managerPropertyChanged()));
  QObject::connect(o, signal, this, SLOT(guiPropertyChanged()));
  this->Block = 0;
}

//-----------------------------------------------------------------------------
void pqPropertyManagerPropertyLink::guiPropertyChanged()
{
  pqPropertyManagerProperty* p = 
    qobject_cast<pqPropertyManagerProperty*>(this->parent());
  QVariant v = this->QtObject->property(QtProperty);
  if(p->value() != v)
    {
    p->setValue(v);
    if(this->Block == 0)
      {
      emit p->guiPropertyChanged();
      }
    }
}

//-----------------------------------------------------------------------------
void pqPropertyManagerPropertyLink::managerPropertyChanged()
{
  ++this->Block;
  pqPropertyManagerProperty* p = 
    qobject_cast<pqPropertyManagerProperty*>(this->parent());
  QVariant v = p->value();
  if(this->QtObject && this->QtObject->property(this->QtProperty) != v)
    {
    this->QtObject->setProperty(this->QtProperty, v);
    }
  --this->Block;
}

//-----------------------------------------------------------------------------
QObject* pqPropertyManagerPropertyLink::object() const
{
  return this->QtObject;
}

//-----------------------------------------------------------------------------
QByteArray pqPropertyManagerPropertyLink::property() const
{
  return this->QtProperty;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
pqPropertyManager::pqPropertyManager(QObject* p)
  : QObject(p)
{
  this->Internal = new pqPropertyManager::pqInternal;
}

//-----------------------------------------------------------------------------
pqPropertyManager::~pqPropertyManager()
{
  foreach(pqPropertyManagerProperty* l, this->Internal->Properties.values())
    {
    delete l;
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqPropertyManager::registerLink(
  QObject* qObject, const char* qProperty, const char* signal,
  vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  if(!Proxy || !Property || !qObject || !qProperty || !signal)
    {
    qWarning("Invalid parameter(s) to register link\n");
    return;
    }

  pqInternal::PropertyMap::iterator iter;
  iter = 
    this->Internal->Properties.find(pqInternal::PropertyKey(Property, Index));
  if(iter == this->Internal->Properties.end())
    {
    pqPropertyManagerProperty* p = new pqPropertyManagerProperty(NULL);
    iter = this->Internal->Properties.insert(
      pqInternal::PropertyKey(Property, Index), p);
   
    this->Internal->Links.addPropertyLink(
      iter.value(), "value", SIGNAL(flushProperty()),
      Proxy, Property, Index);

    QObject::connect(p, SIGNAL(guiPropertyChanged()), 
                     this, SLOT(propertyChanged()));
    QObject::connect(p, SIGNAL(guiPropertyChanged()), 
                     iter.value(), SIGNAL(flushProperty()));
    }
  // link the QObject property with this QObject property
  iter.value()->addLink(qObject, qProperty, signal);
}


//-----------------------------------------------------------------------------
void pqPropertyManager::unregisterLink(
  QObject* qObject, const char* qProperty, const char* signal,
  vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  pqInternal::PropertyMap::iterator iter;
  iter = 
    this->Internal->Properties.find(pqInternal::PropertyKey(Property, Index));
  if(iter != this->Internal->Properties.end())
    {
    iter.value()->removeLink(qObject, qProperty, signal);
    if(iter.value()->numberOfLinks() == 0)
      {
      this->Internal->Links.removePropertyLink(
        iter.value(), "value", SIGNAL(flushProperty()),
        Proxy, Property, Index);
      delete *iter;
      this->Internal->Properties.erase(iter);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPropertyManager::removeAllLinks()
{
  this->Internal->Links.removeAllPropertyLinks();
  foreach (pqPropertyManagerProperty* pqproperty, this->Internal->Properties)
    {
    pqproperty->removeAllLinks();
    delete pqproperty;
    }

  this->Internal->Properties.clear();
}

//-----------------------------------------------------------------------------
void pqPropertyManager::accept()
{
  emit this->aboutToAccept();
  this->Internal->Links.accept();
  emit this->accepted();
  this->Internal->Modified = false;
}

//-----------------------------------------------------------------------------
void pqPropertyManager::reject()
{
  this->Internal->Links.reset();
  emit this->rejected();
  this->Internal->Modified = false;
}

//-----------------------------------------------------------------------------
void pqPropertyManager::propertyChanged()
{
  this->Internal->Modified = true;
  emit this->modified();
}

//-----------------------------------------------------------------------------
bool pqPropertyManager::isModified() const
{
  return this->Internal->Modified;
}
