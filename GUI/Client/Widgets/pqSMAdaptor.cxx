/*=========================================================================

   Program:   ParaQ
   Module:    pqSMAdaptor.cxx

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

#include "pqSMAdaptor.h"

#include <assert.h>
#include <vtkstd/map>

#include <QString>
#include <QVariant>
#include <QByteArray>
#include <QSignalMapper>

#include "vtkEventQtSlotConnect.h"

#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDomain.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyGroupDomain.h"

#include "pqSMProxy.h"


namespace {
  // store SM side of connection information
  class SMGroup
    {
  public:
    SMGroup(vtkSMProxy* a, vtkSMProperty* b, int c)
      : Proxy(a), Property(b), Index(c) {}
    SMGroup& operator=(const SMGroup& copy)
      {
      this->Proxy = copy.Proxy;
      this->Property = copy.Property;
      this->Index = copy.Index;
      return *this;
      }
    bool operator<(SMGroup const& other) const
      {
      if(this->Proxy < other.Proxy)
        {
        return true;
        }
      else if(this->Proxy > other.Proxy)
        {
        return false;
        }

      if(this->Property < other.Property)
        {
        return true;
        }
      else if(this->Property > other.Property)
        {
        return false;
        }

      return this->Index < other.Index;
      }
    vtkSMProxy* Proxy;
    vtkSMProperty* Property;
    int Index;
    };
  // store Qt side of connection information
  typedef vtkstd::pair<QObject*, QByteArray> QtGroup;
}


class pqSMAdaptorInternal
{
public:
  pqSMAdaptorInternal()
    {
    this->VTKConnections = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->QtConnections = new QSignalMapper;
    this->DoingSMPropertyModified = false;
    this->DoingQtPropertyModified = false;
    }
  ~pqSMAdaptorInternal()
    {
    delete this->QtConnections;
    }

  // managed links
  typedef vtkstd::multimap<SMGroup, QtGroup> LinkMap;
  LinkMap SMLinks;
  LinkMap QtLinks;

  // handle changes from the SM side
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnections;
  
  // handle changes from the Qt side
  QSignalMapper* QtConnections;

  // handle domain connections
  typedef vtkstd::multimap<vtkSMProperty*, vtkstd::pair<QObject*, QByteArray> > DomainConnectionMap;
  DomainConnectionMap DomainConnections;

  static bool SettingMultipleProperty;
  bool DoingSMPropertyModified;
  bool DoingQtPropertyModified;
};

bool pqSMAdaptorInternal::SettingMultipleProperty = false;

pqSMAdaptor *pqSMAdaptor::Instance = 0;


pqSMAdaptor::pqSMAdaptor()
{

  this->Internal = new pqSMAdaptorInternal;
  QObject::connect(this->Internal->QtConnections, SIGNAL(mapped(QWidget*)), 
                   this, SLOT(qtLinkedPropertyChanged(QWidget*)));

  if(!pqSMAdaptor::Instance)
    {
    pqSMAdaptor::Instance = this;
    }
}

pqSMAdaptor::~pqSMAdaptor()
{
  if(pqSMAdaptor::Instance == this)
    {
    pqSMAdaptor::Instance = 0;
    }

  delete this->Internal;
}

pqSMAdaptor* pqSMAdaptor::instance()
{
  return pqSMAdaptor::Instance;
}

pqSMAdaptor::PropertyType pqSMAdaptor::getPropertyType(vtkSMProperty* Property)
{

  pqSMAdaptor::PropertyType type = pqSMAdaptor::UNKNOWN;
  if(!Property)
    {
    return type;
    }

  vtkSMProxyProperty* proxy = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxy)
    {
    vtkSMInputProperty* input = vtkSMInputProperty::SafeDownCast(Property);
    if(input && input->GetMultipleInput())
      {
      type = pqSMAdaptor::PROXYLIST;
      }
    type = pqSMAdaptor::PROXY;
    }
  else
    {
    vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
    adaptor->SetProperty(Property);

    if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
      {
      type = pqSMAdaptor::SELECTION;
      }
    else if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::ENUMERATION)
      {
      type = pqSMAdaptor::ENUMERATION;
      }
    else if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::FILE_LIST)
      {
      type = pqSMAdaptor::FILE_LIST;
      }
    else 
      {
      vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);

      assert(VectorProperty != NULL);
      if(VectorProperty && VectorProperty->GetNumberOfElements() > 1)
        {
        type = pqSMAdaptor::MULTIPLE_ELEMENTS;
        }
      else if(VectorProperty && VectorProperty->GetNumberOfElements() == 1)
        {
        type = pqSMAdaptor::SINGLE_ELEMENT;
        }
      }
    adaptor->Delete();
    }

  // make sure we can know about all types
  assert(type != pqSMAdaptor::UNKNOWN);

  return type;
}

pqSMProxy pqSMAdaptor::getProxyProperty(vtkSMProxy* Proxy, vtkSMProperty* Property)
{
  Proxy->UpdatePropertyInformation(Property);

  if(pqSMAdaptor::getPropertyType(Property) == pqSMAdaptor::PROXY)
    {
    vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
    if(proxyProp->GetNumberOfProxies())
      {
      return pqSMProxy(proxyProp->GetProxy(0));
      }
    else
      {
      // no proxy property defined and one is required, so go find one to set
      QList<pqSMProxy> domain = pqSMAdaptor::getProxyPropertyDomain(Proxy, Property);   // TODO fix this
      if(domain.size())
        {
        //this->setProxyProperty(Proxy, Property, domain[0]);
        return domain[0];
        }
      }
    }
  return pqSMProxy(NULL);
}

void pqSMAdaptor::setProxyProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, pqSMProxy Value)
{
  if(pqSMAdaptor::getPropertyType(Property) == pqSMAdaptor::PROXY)
    {
    vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
    proxyProp->RemoveAllProxies();
    proxyProp->AddProxy(Value);
    }
  Proxy->UpdateVTKObjects();
}

QList<pqSMProxy> pqSMAdaptor::getProxyListProperty(vtkSMProxy* Proxy, vtkSMProperty* Property)
{
  Proxy->UpdatePropertyInformation(Property);

  QList<pqSMProxy> value;
  if(pqSMAdaptor::getPropertyType(Property) == pqSMAdaptor::PROXYLIST)
    {
    vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
    unsigned int num = proxyProp->GetNumberOfProxies();
    for(unsigned int i=0; i<num; i++)
      {
      value.append(proxyProp->GetProxy(i));
      }
    }
  return value;
}

void pqSMAdaptor::setProxyListProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QList<pqSMProxy> Value)
{
  if(pqSMAdaptor::getPropertyType(Property) == pqSMAdaptor::PROXYLIST)
    {
    vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
    proxyProp->RemoveAllProxies();
    foreach(pqSMProxy p, Value)
      {
      proxyProp->AddProxy(p);
      }
    }
  Proxy->UpdateVTKObjects();
}

QList<pqSMProxy> pqSMAdaptor::getProxyPropertyDomain(vtkSMProxy* Proxy, vtkSMProperty* Property)
{
  Property->UpdateDependentDomains();

  QList<pqSMProxy> proxydomain;
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    vtkSMProxyManager* pm = vtkSMProxyManager::GetProxyManager();
    
    // get group domain of this property and add all proxies in those groups to our list
    vtkSMProxyGroupDomain* gd = vtkSMProxyGroupDomain::SafeDownCast(Property->GetDomain("groups"));
    if(gd)
      {
      unsigned int numGroups = gd->GetNumberOfGroups();
      for(unsigned int i=0; i<numGroups; i++)
        {
        const char* group = gd->GetGroup(i);
        unsigned int numProxies = pm->GetNumberOfProxies(group);
        for(unsigned int j=0; j<numProxies; j++)
          {
          pqSMProxy p = pm->GetProxy(group, pm->GetProxyName(group, j));
          if(p != Proxy)
            {
            proxydomain.append(p);
            }
          }
        }
      }

    // TODO: consider other domains
    // possible domains are vtkSMInputArrayDomain, vtkSMProxyGroupDomain, vtkSMFixedTypeDomain, vtkSMDataTypeDomain
    
    }
  return proxydomain;
}


QList<QList<QVariant> > pqSMAdaptor::getSelectionProperty(vtkSMProxy* Proxy, vtkSMProperty* Property)
{
  QList<QList<QVariant> > val;

  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
    {
    int numElems = adaptor->GetNumberOfSelectionElements();
    for(int i=0; i<numElems; i++)
      {
      QString name = adaptor->GetSelectionName(i);
      QVariant var;
      vtkSMStringVectorProperty* infoProp = vtkSMStringVectorProperty::SafeDownCast(Property->GetInformationProperty());
      if(infoProp)
        {
        Proxy->UpdatePropertyInformation(infoProp);
        int exists;
        int idx = infoProp->GetElementIndex(name.toAscii().data(), exists);
        var = infoProp->GetElement(idx+1);
        }
      else
        {
        var = adaptor->GetSelectionValue(i);
        }
      
      // Convert the variant to the appropriate type.
      switch(adaptor->GetElementType())
        {
        case vtkSMPropertyAdaptor::INT:
          {
          if(var.canConvert(QVariant::Int))
            {
            var.convert(QVariant::Int);
            }
          }
          break;
        case vtkSMPropertyAdaptor::DOUBLE:
          {
          if(var.canConvert(QVariant::Double))
            {
            var.convert(QVariant::Double);
            }
          }
          break;
        case vtkSMPropertyAdaptor::BOOLEAN:
          {
          if(var.canConvert(QVariant::Bool))
            {
            var.convert(QVariant::Bool);
            }
          }
          break;
        }
      QList<QVariant> newvar;
      newvar.push_back(name);
      newvar.push_back(var);
      val.push_back(newvar);
      }
    }
  adaptor->Delete();

  return val;
}

QList<QVariant> pqSMAdaptor::getSelectionProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  QList<QVariant> val;

  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
    {
    int numElems = adaptor->GetNumberOfSelectionElements();
    if(Index < numElems)
      {
      QString name = adaptor->GetSelectionName(Index);
      QVariant var;
      vtkSMStringVectorProperty* infoProp = vtkSMStringVectorProperty::SafeDownCast(Property->GetInformationProperty());
      if(infoProp)
        {
        Proxy->UpdatePropertyInformation(infoProp);
        int exists;
        int idx = infoProp->GetElementIndex(name.toAscii().data(), exists);
        var = infoProp->GetElement(idx+1);
        }
      else
        {
        var = adaptor->GetSelectionValue(Index);
        }
      
      // Convert the variant to the appropriate type.
      switch(adaptor->GetElementType())
        {
        case vtkSMPropertyAdaptor::INT:
          {
          if(var.canConvert(QVariant::Int))
            {
            var.convert(QVariant::Int);
            }
          }
          break;
        case vtkSMPropertyAdaptor::DOUBLE:
          {
          if(var.canConvert(QVariant::Double))
            {
            var.convert(QVariant::Double);
            }
          }
          break;
        case vtkSMPropertyAdaptor::BOOLEAN:
          {
          if(var.canConvert(QVariant::Bool))
            {
            var.convert(QVariant::Bool);
            }
          }
          break;
        }
      val.push_back(name);
      val.push_back(var);
      }
    }
  adaptor->Delete();

  return val;
}

void pqSMAdaptor::setSelectionProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QList<QList<QVariant> > Value)
{
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
    {

    QList<QVariant> domain = pqSMAdaptor::getSelectionPropertyDomain(Property);

    foreach(QList<QVariant> l, Value)
      {
      if(l.size() < 2)
        {
        continue;
        }

      QString name = l[0].toString();
      QVariant value = l[1];
      if(value.type() == QVariant::Bool)
        {
        value = value.toInt();
        }

      for(int i=0; i<domain.size(); i++)
        {
        if(domain[i] == name)
          {
          adaptor->SetSelectionValue(i, value.toString().toAscii().data());
          }
        }
      
      /*
      pqSMAdaptorInternal::SettingMultipleProperty = true;
      adaptor->SetRangeValue(0, name.toAscii().data());
      adaptor->SetRangeValue(1, value.toString().toAscii().data());
      */
      Proxy->UpdateVTKObjects();
      /*
      pqSMAdaptorInternal::SettingMultipleProperty = false;
      Property->Modified();  // let ourselves know it was modified, since we blocked it previously
      */
      }
    }
  adaptor->Delete();
}

void pqSMAdaptor::setSelectionProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int vtkNotUsed(Index), QList<QVariant> Value)
{
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
    {

    QList<QVariant> domain = pqSMAdaptor::getSelectionPropertyDomain(Property);

    if(Value.size() == 2)
      {
      QString name = Value[0].toString();
      QVariant value = Value[1];
      if(value.type() == QVariant::Bool)
        {
        value = value.toInt();
        }

      for(int i=0; i<domain.size(); i++)
        {
        if(domain[i] == name)
          {
          adaptor->SetSelectionValue(i, value.toString().toAscii().data());
          }
        }
      
      /*
      pqSMAdaptorInternal::SettingMultipleProperty = true;
      adaptor->SetRangeValue(0, name.toAscii().data());
      adaptor->SetRangeValue(1, value.toString().toAscii().data());
      */
      Proxy->UpdateVTKObjects();
      /*
      pqSMAdaptorInternal::SettingMultipleProperty = false;
      Property->Modified();  // let ourselves know it was modified, since we blocked it previously
      */
      }
    }
  adaptor->Delete();
}

QList<QVariant> pqSMAdaptor::getSelectionPropertyDomain(vtkSMProperty* Property)
{
  Property->UpdateDependentDomains();
  QList<QVariant> prop;
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(vtkSMPropertyAdaptor::SELECTION == adaptor->GetPropertyType())
    {
    int num = adaptor->GetNumberOfSelectionElements();
    for(int i=0; i<num; i++)
      {
      prop.append(adaptor->GetSelectionName(i));
      }
    }
  adaptor->Delete();
  return prop;
}
  
QVariant pqSMAdaptor::getEnumerationProperty(vtkSMProxy* Proxy, vtkSMProperty* Property)
{
  Proxy->UpdatePropertyInformation(Property);
  QVariant var;

  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(adaptor->GetNumberOfEnumerationElements())
    {
    if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::ENUMERATION)
      {
      var = adaptor->GetEnumerationValue();
      var = adaptor->GetEnumerationName(var.toInt());
      }

    if(adaptor->GetElementType() == vtkSMPropertyAdaptor::BOOLEAN)
      {
      var.convert(QVariant::Int);
      var.convert(QVariant::Bool);
      }
    }
  
  adaptor->Delete();

  return var;
}

void pqSMAdaptor::setEnumerationProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QVariant Value)
{
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::ENUMERATION &&
     adaptor->GetElementType() == vtkSMPropertyAdaptor::BOOLEAN)
    {
    Value.convert(QVariant::Int);
    adaptor->SetEnumerationValue(Value.toString().toAscii().data());
    Proxy->UpdateVTKObjects();
    }
  else if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::ENUMERATION)
    {
    QString val = Value.toString();
    int num = adaptor->GetNumberOfEnumerationElements();
    for(int i=0; i<num; i++)
      {
      if(val == adaptor->GetEnumerationName(i))
        {
        val.setNum(i);
        adaptor->SetEnumerationValue(val.toAscii().data());
        i = num;
        }
      }
    Proxy->UpdateVTKObjects();
    }
  adaptor->Delete();
}

QList<QVariant> pqSMAdaptor::getEnumerationPropertyDomain(vtkSMProperty* Property)
{
  Property->UpdateDependentDomains();
  QList<QVariant> enumerations;

  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);

  if(vtkSMPropertyAdaptor::ENUMERATION == adaptor->GetPropertyType())
    {
    int num = adaptor->GetNumberOfEnumerationElements();
    for(int i=0; i<num; i++)
      {
      enumerations.append(adaptor->GetEnumerationName(i));
      }
    }
  adaptor->Delete();

  return enumerations;
}

QVariant pqSMAdaptor::getElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property)
{
  Proxy->UpdatePropertyInformation(Property);
  QVariant var;
  
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  if(VectorProperty)
    {
    vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
    adaptor->SetProperty(Property);

    var = adaptor->GetRangeValue(0);
    
    // Convert the variant to the appropriate type.
    switch(adaptor->GetElementType())
      {
      case vtkSMPropertyAdaptor::INT:
        {
        if(var.canConvert(QVariant::Int))
          {
          var.convert(QVariant::Int);
          }
        }
        break;
      case vtkSMPropertyAdaptor::DOUBLE:
        {
        if(var.canConvert(QVariant::Double))
          {
          var.convert(QVariant::Double);
          }
        }
        break;
      case vtkSMPropertyAdaptor::BOOLEAN:
        {
        if(var.canConvert(QVariant::Bool))
          {
          var.convert(QVariant::Bool);
          }
        }
        break;
      }

    adaptor->Delete();
    }
  return var;
}

void pqSMAdaptor::setElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QVariant Value)
{
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  if(VectorProperty)
    {
    vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
    adaptor->SetProperty(Property);

    if(Value.type() == QVariant::Bool)
      {
      Value = Value.toInt();
      }
    
    adaptor->SetRangeValue(0, Value.toString().toAscii().data());
    adaptor->Delete();
    Proxy->UpdateVTKObjects();
    }
}

QList<QVariant> pqSMAdaptor::getElementPropertyDomain(vtkSMProperty* Property)
{
  Property->UpdateDependentDomains();
  QList<QVariant> domain;
  
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);

  const char* min = adaptor->GetRangeMinimum(0);
  const char* max = adaptor->GetRangeMaximum(0);

  if(min && max)
    {
    domain.push_back(min);
    domain.push_back(max);
    }
  
  adaptor->Delete();

  return domain;
}
  
QList<QVariant> pqSMAdaptor::getMultipleElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property)
{
  Proxy->UpdatePropertyInformation(Property);
  QList<QVariant> props;
  
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  if(!VectorProperty)
    {
    return props;
    }

  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);

  int i;
  int num = VectorProperty->GetNumberOfElements();
  for(i=0; i<num; i++)
    {
    props.push_back(pqSMAdaptor::getMultipleElementProperty(Proxy, Property, i));
    }

  adaptor->Delete();

  return props;
}

void pqSMAdaptor::setMultipleElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QList<QVariant> Value)
{
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  if(!VectorProperty)
    {
    return;
    }

  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);

  int i;
  int num = VectorProperty->GetNumberOfElements();
  if(Value.size() < num)
    {
    num = Value.size();
    }

  for(i=0; i<num; i++)
    {
    QVariant val = Value[i];
    if(val.type() == QVariant::Bool)
      {
      val = val.toInt();
      }
    adaptor->SetRangeValue(i, val.toString().toAscii().data());
    }
  Proxy->UpdateVTKObjects();
  adaptor->Delete();
}

QList<QList<QVariant> > pqSMAdaptor::getMultipleElementPropertyDomain(vtkSMProperty* Property)
{
  Property->UpdateDependentDomains();
  QList<QList<QVariant> > domains;
  
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  
  if(vtkSMPropertyAdaptor::RANGE == adaptor->GetPropertyType())
    {
    int num = adaptor->GetNumberOfRangeElements();
    for(int i=0; i<num; i++)
      {
      QList<QVariant> domain;
      const char* min = adaptor->GetRangeMinimum(0);
      const char* max = adaptor->GetRangeMaximum(0);
      if(min && max)
        {
        domain.push_back(min);
        domain.push_back(max);
        }
      domains.append(domain);
      }
    }

  adaptor->Delete();

  return domains;
}

QVariant pqSMAdaptor::getMultipleElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  Proxy->UpdatePropertyInformation(Property);
  QVariant var;
  
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  if(VectorProperty)
    {
    vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
    adaptor->SetProperty(Property);

    var = adaptor->GetRangeValue(Index);
    
    // Convert the variant to the appropriate type.
    switch(adaptor->GetElementType())
      {
      case vtkSMPropertyAdaptor::INT:
        {
        if(var.canConvert(QVariant::Int))
          {
          var.convert(QVariant::Int);
          }
        }
        break;
      case vtkSMPropertyAdaptor::DOUBLE:
        {
        if(var.canConvert(QVariant::Double))
          {
          var.convert(QVariant::Double);
          }
        }
        break;
      case vtkSMPropertyAdaptor::BOOLEAN:
        {
        if(var.canConvert(QVariant::Bool))
          {
          var.convert(QVariant::Bool);
          }
        }
        break;
      }

    adaptor->Delete();
    }
  return var;
}

void pqSMAdaptor::setMultipleElementProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index, QVariant Value)
{
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  if(VectorProperty)
    {
    vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
    adaptor->SetProperty(Property);

    if(Value.type() == QVariant::Bool)
      Value = Value.toInt();
    
    adaptor->SetRangeValue(Index, Value.toString().toAscii().data());
    Proxy->UpdateVTKObjects();
    adaptor->Delete();
    }
}

QList<QVariant> pqSMAdaptor::getMultipleElementPropertyDomain(vtkSMProperty* Property, int Index)
{
  Property->UpdateDependentDomains();
  QList<QVariant> domain;
  
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  
  if(vtkSMPropertyAdaptor::RANGE == adaptor->GetPropertyType())
    {
    const char* min = adaptor->GetRangeMinimum(Index);
    const char* max = adaptor->GetRangeMaximum(Index);
    if(min && max)
      {
      domain.push_back(min);
      domain.push_back(max);
      }
    }

  adaptor->Delete();

  return domain;
}

QString pqSMAdaptor::getFileListProperty(vtkSMProxy* Proxy, vtkSMProperty* Property)
{
  Proxy->UpdatePropertyInformation(Property);
  QString file;
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::FILE_LIST)
    {
    file = adaptor->GetRangeValue(0);
    }
  adaptor->Delete();
  return file;
}

void pqSMAdaptor::setFileListProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QString Value)
{
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::FILE_LIST)
    {
    adaptor->SetRangeValue(0, Value.toAscii().data());
    }
  adaptor->Delete();
  Proxy->UpdateVTKObjects();
}


void pqSMAdaptor::setProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, QVariant QtProperty)
{
  QList<QVariant> props;
  if(QtProperty.type() == QVariant::List)
    {
    props = QtProperty.toList();
    }
  else
    {
    props.push_back(QtProperty);
    }

  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  assert(VectorProperty != NULL);
  if(!VectorProperty)
    {
    return;
    }
  
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);

  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
    {
    assert(props.size() <= (QList<QVariant>::size_type)adaptor->GetNumberOfSelectionElements());
    }
  else
    {
    assert(props.size() <= (QList<QVariant>::size_type)VectorProperty->GetNumberOfElements());
    }

  for(int i=0; i<props.size(); i++)
    {
    this->setProperty(Proxy, Property, i, props[i]);
    }
  adaptor->Delete();
}

QVariant pqSMAdaptor::getProperty(vtkSMProxy* Proxy, vtkSMProperty* Property)
{
  vtkSMVectorProperty* VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  if(!VectorProperty)
    {
    return QVariant();
    }

  int numElems = 0;
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);

  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
    {
    numElems = adaptor->GetNumberOfSelectionElements();
    }
  else
    {
    numElems = VectorProperty->GetNumberOfElements();
    if(numElems == 1)
      {
      QVariant var = this->getProperty(Proxy, Property, 0);
      adaptor->Delete();
      return var;
      }
    }

  QList<QVariant> props;
  for(int i=0; i<numElems; i++)
    {
    props.push_back(this->getProperty(Proxy, Property, i));
    }
  
  adaptor->Delete();
  return props;
}

void pqSMAdaptor::setProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index, QVariant QtProperty)
{
  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::ENUMERATION &&
      adaptor->GetElementType() == vtkSMPropertyAdaptor::INT)
    {
    adaptor->SetEnumerationValue(QtProperty.toString().toAscii().data());
    Proxy->UpdateVTKObjects();
    }
  else if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION && Property->GetInformationProperty())
    {
    QString name;
    QVariant value;
    // support two ways of setting selection properties
    // TODO: review this and pick one way to do it?
    // I added this first one so it was easier to set properties from code that doesn't have an index
    if(QtProperty.type() == QVariant::List)
      {
      name = QtProperty.toList()[0].toString();
      value = QtProperty.toList()[1];
      if(value.type() == QVariant::Bool)
        value = value.toInt();
      }
    else
      {
      name = adaptor->GetSelectionName(Index);
      value = QtProperty;
      if(value.type() == QVariant::Bool)
        value = value.toInt();
      }
    this->Internal->SettingMultipleProperty = true;
    adaptor->SetRangeValue(0, name.toAscii().data());
    adaptor->SetRangeValue(1, value.toString().toAscii().data());
    Proxy->UpdateVTKObjects();
    this->Internal->SettingMultipleProperty = false;
    Property->Modified();  // let ourselves know it was modified, since we blocked it previously
    }
  else
    {
    // bools expand to "true" or "false" instead of "1" or "0"
    if(QtProperty.type() == QVariant::Bool)
      QtProperty = QtProperty.toInt();
    adaptor->SetRangeValue(Index, QtProperty.toString().toAscii().data());
    Proxy->UpdateVTKObjects();
    }

  adaptor->Delete();
}

QVariant pqSMAdaptor::getProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  Proxy->UpdatePropertyInformation(Property);
  QVariant var;

  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);

  int propertyType = adaptor->GetPropertyType();
  QString name;

  if(vtkSMPropertyAdaptor::SELECTION == propertyType)
    {
    name = adaptor->GetSelectionName(Index);
    vtkSMStringVectorProperty* infoProp = vtkSMStringVectorProperty::SafeDownCast(Property->GetInformationProperty());
    if(infoProp)
      {
      Proxy->UpdatePropertyInformation(infoProp);
      int exists;
      int idx = infoProp->GetElementIndex(name.toAscii().data(), exists);
      var = infoProp->GetElement(idx+1);
      }
    else
      {
      var = adaptor->GetSelectionValue(Index);
      }
    }
  else
    {
    var = adaptor->GetRangeValue(Index);
    }

  // Convert the variant to the appropriate type.
  switch(adaptor->GetElementType())
    {
    case vtkSMPropertyAdaptor::INT:
      {
      if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::ENUMERATION)
        {
        var = adaptor->GetEnumerationValue();
        }
      if(var.canConvert(QVariant::Int))
        {
        var.convert(QVariant::Int);
        }
      }
      break;
    case vtkSMPropertyAdaptor::DOUBLE:
      {
      if(var.canConvert(QVariant::Double))
        {
        var.convert(QVariant::Double);
        }
      }
      break;
    case vtkSMPropertyAdaptor::BOOLEAN:
      {
      if(var.canConvert(QVariant::Bool))
        {
        var.convert(QVariant::Bool);
        }
      }
      break;
    }

  adaptor->Delete();

  if(!name.isNull())
    {
    QList<QVariant> newvar;
    newvar.push_back(name);
    newvar.push_back(var);
    var = newvar;
    }
  return var;
}

QVariant pqSMAdaptor::getPropertyDomain(vtkSMProperty* Property)
{
  QVariant prop;

  Property->UpdateDependentDomains();

  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  
  int propertyType = adaptor->GetPropertyType();
  if(vtkSMPropertyAdaptor::SELECTION == propertyType)
    {
    int num = adaptor->GetNumberOfSelectionElements();
    QList<QVariant> selections;
    for(int i=0; i<num; i++)
      {
      selections.append(adaptor->GetSelectionName(i));
      }
    prop = selections;
    }
  else if(vtkSMPropertyAdaptor::ENUMERATION == propertyType)
    {
    int num = adaptor->GetNumberOfEnumerationElements();
    QList<QVariant> enumerations;
    for(int i=0; i<num; i++)
      {
      QVariant e = adaptor->GetEnumerationName(i);
      enumerations.append(e);
      }
    prop = enumerations;
    }
  else if(vtkSMPropertyAdaptor::RANGE == propertyType)
    {
    int num = adaptor->GetNumberOfRangeElements();
    QList<QVariant> ranges;
    for(int i=0; i<num; i++)
      {
      QVariant e = adaptor->GetRangeMinimum(i);
      ranges.append(e);
      e = adaptor->GetRangeMaximum(i);
      ranges.append(e);
      }
    prop = ranges;
    }
  
  adaptor->Delete();
  return prop;
}

void pqSMAdaptor::linkPropertyTo(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index,
                                        QObject* qObject, const char* qProperty)
{
  if(!Property || !qObject)
    return;

  // set the property on the QObject, so they start in-sync
  QVariant val;
  if(this->getPropertyType(Property) == pqSMAdaptor::PROXY)
    {
    val.setValue(this->getProxyProperty(Proxy, Property));
    }
  else
    {
    val = this->getProperty(Proxy, Property, Index);
    }
  if(val.type() == QVariant::List)
    {
    val = val.toList()[1];
    }
  qObject->setProperty(qProperty, val);

  pqSMAdaptorInternal::LinkMap::iterator iter = this->Internal->SMLinks.find(SMGroup(Proxy, Property, Index));
  bool found = iter != this->Internal->SMLinks.end();
    
  iter = this->Internal->SMLinks.insert(iter, 
                               pqSMAdaptorInternal::LinkMap::value_type(
                                 SMGroup(Proxy, Property, Index), QtGroup(qObject, qProperty)));


  if(!found)
    {
    // connect SM property changed to QObject set property
    this->Internal->VTKConnections->Connect(Property, vtkCommand::ModifiedEvent,
                                            this, SLOT(smLinkedPropertyChanged(vtkObject*, unsigned long, void*)),
                                            (void*)&(iter->first));
    }
}

void pqSMAdaptor::unlinkPropertyFrom(vtkSMProxy* Proxy, vtkSMProperty* Property, int Index,
                                            QObject* qObject, const char* qProperty)
{
  typedef vtkstd::pair<pqSMAdaptorInternal::LinkMap::iterator,
                       pqSMAdaptorInternal::LinkMap::iterator> Iters;
  
  Iters iters = this->Internal->SMLinks.equal_range(SMGroup(Proxy, Property, Index));

  bool all = true;

  pqSMAdaptorInternal::LinkMap::iterator qiter;
  for(qiter = iters.first; qiter != iters.second; )
    {
    if((qObject == NULL || qiter->second.first == qObject) &&
       (qProperty == NULL || qiter->second.second == qProperty))
      {
      this->Internal->SMLinks.erase(qiter++);
      }
    else
      {
      ++qiter;
      all = false;
      }
    }
  
  if(all)
    {
    this->Internal->VTKConnections->Disconnect(Property, vtkCommand::ModifiedEvent, this);
    }
}

void pqSMAdaptor::linkPropertyTo(QObject* qObject, const char* qProperty, const char* signal,
                                        vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  if(!Proxy || !Property || !qObject)
    return;

  QVariant val;
  if(this->getPropertyType(Property) == pqSMAdaptor::PROXY)
    {
    val.setValue(this->getProxyProperty(Proxy, Property));
    }
  else
    {
    val = this->getProperty(Proxy, Property, Index);
    if(val.type() == QVariant::List)
      {
      val = val.toList()[1];
      }
    }
  qObject->setProperty(qProperty, val);

  pqSMAdaptorInternal::LinkMap::iterator iter = this->Internal->QtLinks.insert(this->Internal->QtLinks.end(), 
                               pqSMAdaptorInternal::LinkMap::value_type(
                                 SMGroup(Proxy, Property, Index), QtGroup(qObject, qProperty)));

  QObject::connect(qObject, signal, this->Internal->QtConnections, SLOT(map()));
  this->Internal->QtConnections->setMapping(qObject, reinterpret_cast<QWidget*>(&*iter));

}

void pqSMAdaptor::unlinkPropertyFrom(QObject* qObject, const char* qProperty, const char* signal,
                                            vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  typedef vtkstd::pair<pqSMAdaptorInternal::LinkMap::iterator,
                       pqSMAdaptorInternal::LinkMap::iterator> Iters;
  
  Iters iters = this->Internal->QtLinks.equal_range(SMGroup(Proxy, Property, Index));

  bool all = true;

  pqSMAdaptorInternal::LinkMap::iterator qiter;
  for(qiter = iters.first; qiter != iters.second; )
    {
    if((qObject == NULL || qiter->second.first == qObject) &&
       (qProperty == NULL || qiter->second.second == qProperty))
      {
      QObject::disconnect(qiter->second.first, signal, this->Internal->QtConnections, SLOT(map()));
      this->Internal->QtConnections->removeMappings(qiter->second.first);
      this->Internal->QtLinks.erase(qiter++);
      }
    else
      {
      ++qiter;
      all = false;
      }
    }
}

void pqSMAdaptor::smLinkedPropertyChanged(vtkObject*, unsigned long, void* data)
{
  if(this->Internal->SettingMultipleProperty == true)
    {
    // when setting properties on selection properties, this slot gets called in the middle of
    // setting the value, so it messes up the state of things
    return;
    }
  
  if(this->Internal->DoingQtPropertyModified == true)
    {
    // prevent recursion
    return;
    }

  this->Internal->DoingSMPropertyModified = true;
  

  SMGroup* d = static_cast<SMGroup*>(data);
  
  typedef vtkstd::pair<pqSMAdaptorInternal::LinkMap::iterator,
                       pqSMAdaptorInternal::LinkMap::iterator> Iters;
  pqSMAdaptorInternal::LinkMap::iterator iter;
  
  // is there a way to not do a lookup?
  Iters iters = this->Internal->SMLinks.equal_range(*d);
  
  QVariant var;
 
  if(this->getPropertyType(d->Property) == pqSMAdaptor::PROXY)
    {
    var.setValue(this->getProxyProperty(d->Proxy, d->Property));
    }
  else
    {
    var = this->getProperty(d->Proxy, d->Property, d->Index);
    }

  if(var.type() == QVariant::List)
    {
    var = var.toList()[1];
    }
  
  for(iter = iters.first; iter != iters.second; ++iter)
    {
    QVariant old = iter->second.first->property(iter->second.second.data());
    if(old.type() == QVariant::List)
      {
      old = old.toList()[1];
      }
    old.convert(var.type());
    if(old != var)
      iter->second.first->setProperty(iter->second.second.data(), var);
    }
  this->Internal->DoingSMPropertyModified = false;
}

void pqSMAdaptor::qtLinkedPropertyChanged(QWidget* data)
{
  if(this->Internal->DoingSMPropertyModified == true)
    {
    // prevent recursion
    return;
    }

  this->Internal->DoingQtPropertyModified = true;

  // map::value_type is masked as a QWidget
  pqSMAdaptorInternal::LinkMap::value_type* iter = 
    reinterpret_cast<pqSMAdaptorInternal::LinkMap::value_type*>(data);
  
  QVariant prop = iter->second.first->property(iter->second.second.data());
  QVariant old;
  if(prop.value<pqSMProxy>())
    {
    old.setValue(this->getProxyProperty(iter->first.Proxy, iter->first.Property));
    }
  else
    {
    old = this->getProperty(iter->first.Proxy, iter->first.Property, iter->first.Index);
    if(old.type() == QVariant::List)
      {
      QList<QVariant> tmp = old.toList();
      old = tmp[1];
      }
    if(prop.type() == QVariant::List)
      {
      prop = prop.toList()[1];
      }
    old.convert(prop.type());
    }

  if(prop != old)
    {
    this->setProperty(iter->first.Proxy, iter->first.Property, iter->first.Index, prop);
    iter->first.Proxy->UpdateVTKObjects();
    }
  this->Internal->DoingQtPropertyModified = false;
}

void pqSMAdaptor::connectDomain(vtkSMProperty* prop, QObject* qObject, const char* slot)
{
  if(!QMetaObject::checkConnectArgs(SIGNAL(foo(vtkSMProperty*)), slot))
    {
    qWarning("Incorrect slot %s::%s for pqSMAdaptor::ConnectDomain\n", 
              qObject->metaObject()->className(),
              slot+1);
    return;
    }

  vtkSMDomainIterator* domainIter = prop->NewDomainIterator();
  for(; !domainIter->IsAtEnd(); domainIter->Next())
    {
    pqSMAdaptorInternal::DomainConnectionMap::iterator iter =
      this->Internal->DomainConnections.insert(this->Internal->DomainConnections.end(),
                                               pqSMAdaptorInternal::DomainConnectionMap::value_type(
                                                 prop, vtkstd::pair<QObject*, QByteArray>(
                                                   qObject, slot)));
    this->Internal->VTKConnections->Connect(domainIter->GetDomain(), vtkCommand::DomainModifiedEvent,
                                            this, SLOT(smDomainChanged(vtkObject*, unsigned long, void*)),
                                            &*iter);
    }
  domainIter->Delete();
}

void pqSMAdaptor::disconnectDomain(vtkSMProperty* prop, QObject* qObject, const char* slot)
{
  typedef vtkstd::pair<pqSMAdaptorInternal::DomainConnectionMap::iterator, pqSMAdaptorInternal::DomainConnectionMap::iterator> PairIter;

  PairIter iters = this->Internal->DomainConnections.equal_range(prop);

  for(; iters.first != iters.second; ++iters.first)
    {
    if(iters.first->second.first == qObject && iters.first->second.second == slot)
      {
      vtkSMDomainIterator* domainIter = prop->NewDomainIterator();
      for(; !domainIter->IsAtEnd(); domainIter->Next())
        {
        this->Internal->VTKConnections->Disconnect(domainIter->GetDomain(), vtkCommand::DomainModifiedEvent,
                                                   this, SLOT(smDomainChanged(vtkObject*, unsigned long, void*)),
                                                   &*iters.first);
        this->Internal->DomainConnections.erase(iters.first);
        }
      domainIter->Delete();
      return;
      }
    }
}

void pqSMAdaptor::smDomainChanged(vtkObject*, unsigned long /*event*/, void* data)
{
  pqSMAdaptorInternal::DomainConnectionMap::value_type* call = 
    reinterpret_cast<pqSMAdaptorInternal::DomainConnectionMap::value_type*>(data);
  QMetaObject::invokeMethod(call->second.first, call->second.second.data(), Q_ARG(vtkSMProperty*, call->first));
}

