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

#include <QString>
#include <QVariant>

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

pqSMAdaptor::pqSMAdaptor()
{
}

pqSMAdaptor::~pqSMAdaptor()
{
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


