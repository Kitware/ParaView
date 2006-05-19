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

// self includes
#include "pqSMAdaptor.h"

// qt includes
#include <QString>
#include <QVariant>

// vtk includes
#include "vtkConfigure.h"   // for 64-bitness

// server manager includes
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
#include "vtkSMStringListRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMArrayListDomain.h"

// paraq includes
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
      vtkSMVectorProperty* VectorProperty;
      VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);

      Q_ASSERT(VectorProperty != NULL);
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
  Q_ASSERT(type != pqSMAdaptor::UNKNOWN);

  return type;
}

pqSMProxy pqSMAdaptor::getProxyProperty(vtkSMProxy* Proxy, 
                                        vtkSMProperty* Property)
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
      // TODO fix this -- we should do this automatically ??
      // no proxy property defined and one is required, so go find one to set
      QList<pqSMProxy> domain;
      domain = pqSMAdaptor::getProxyPropertyDomain(Proxy, Property);
      if(domain.size())
        {
        //this->setProxyProperty(Proxy, Property, domain[0]);
        return domain[0];
        }
      }
    }
  return pqSMProxy(NULL);
}

void pqSMAdaptor::setProxyProperty(vtkSMProxy* Proxy, vtkSMProperty* Property, 
                                   pqSMProxy Value)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    proxyProp->RemoveAllProxies();
    proxyProp->AddProxy(Value);
    Proxy->UpdateVTKObjects();
    }
}

void pqSMAdaptor::setUncheckedProxyProperty(vtkSMProxy* vtkNotUsed(Proxy), 
                                   vtkSMProperty* Property,
                                   pqSMProxy Value)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    proxyProp->RemoveAllUncheckedProxies();
    proxyProp->AddUncheckedProxy(Value);
    proxyProp->UpdateDependentDomains();
    }
}

QList<pqSMProxy> pqSMAdaptor::getProxyListProperty(vtkSMProxy* Proxy, 
                                                   vtkSMProperty* Property)
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

void pqSMAdaptor::setProxyListProperty(vtkSMProxy* Proxy, 
                  vtkSMProperty* Property, QList<pqSMProxy> Value)
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

QList<pqSMProxy> pqSMAdaptor::getProxyPropertyDomain(vtkSMProxy* Proxy, 
                                                     vtkSMProperty* Property)
{
  Property->UpdateDependentDomains();

  QList<pqSMProxy> proxydomain;
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    vtkSMProxyManager* pm = vtkSMProxyManager::GetProxyManager();
    
    // get group domain of this property 
    // and add all proxies in those groups to our list
    vtkSMProxyGroupDomain* gd;
    gd = vtkSMProxyGroupDomain::SafeDownCast(Property->GetDomain("groups"));
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
    // possible domains are vtkSMInputArrayDomain, vtkSMProxyGroupDomain, 
    // vtkSMFixedTypeDomain, vtkSMDataTypeDomain
    
    }
  return proxydomain;
}


QList<QList<QVariant> > pqSMAdaptor::getSelectionProperty(vtkSMProxy* Proxy, 
                                                    vtkSMProperty* Property)
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
      vtkSMStringVectorProperty* infoProp;
      infoProp = vtkSMStringVectorProperty::SafeDownCast(
                           Property->GetInformationProperty());
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

QList<QVariant> pqSMAdaptor::getSelectionProperty(vtkSMProxy* Proxy, 
                        vtkSMProperty* Property, unsigned int Index)
{
  QList<QVariant> val;

  vtkSMPropertyAdaptor* adaptor = vtkSMPropertyAdaptor::New();
  adaptor->SetProperty(Property);
  if(adaptor->GetPropertyType() == vtkSMPropertyAdaptor::SELECTION)
    {
    unsigned int numElems = adaptor->GetNumberOfSelectionElements();
    if(Index < numElems)
      {
      QString name = adaptor->GetSelectionName(Index);
      QVariant var;
      vtkSMStringVectorProperty* infoProp;
      infoProp = vtkSMStringVectorProperty::SafeDownCast(
                          Property->GetInformationProperty());
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

void pqSMAdaptor::setSelectionProperty(vtkSMProxy* Proxy, 
               vtkSMProperty* Property, QList<QList<QVariant> > Value)
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
      Property->Modified();  
      // let ourselves know it was modified, since we blocked it previously
      */
      }
    }
  adaptor->Delete();
}

void pqSMAdaptor::setUncheckedSelectionProperty(vtkSMProxy* vtkNotUsed(Proxy), 
               vtkSMProperty* Property, QList<QList<QVariant> > Value)
{
  vtkSMStringVectorProperty* StringProperty;
  vtkSMStringListRangeDomain* StringDomain = NULL;
  StringProperty = vtkSMStringVectorProperty::SafeDownCast(Property);
  if(StringProperty)
    {
    // TODO   do we need to check the domain?
    vtkSMDomainIterator* iter = Property->NewDomainIterator();
    iter->Begin();
    while(StringDomain == NULL && !iter->IsAtEnd())
      {
      vtkSMDomain* d = iter->GetDomain();
      StringDomain = vtkSMStringListRangeDomain::SafeDownCast(d);
      iter->Next();
      }
    iter->Delete();

    if(StringDomain)
      {
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
        unsigned int numElems;
        numElems = StringProperty->GetNumberOfElements();
        if (numElems % 2 == 0)
          {
          unsigned int i;
          bool wasSet = false;
          for(i=0; i<numElems; i+=2)
            {
            if(name == StringProperty->GetElement(i))
              {
              wasSet = true;
              StringProperty->SetElement(i+1,
                        value.toString().toAscii().data());
              }
            }
          if(!wasSet)
            {
            // TODO ???
            }
          }
        }
      }
    }
  Property->UpdateDependentDomains();
}

void pqSMAdaptor::setSelectionProperty(vtkSMProxy* Proxy, 
                           vtkSMProperty* Property, 
                           QList<QVariant> Value)
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
      Property->Modified();  
      // let ourselves know it was modified, since we blocked it previously
      */
      }
    }
  adaptor->Delete();
}

void pqSMAdaptor::setUncheckedSelectionProperty(vtkSMProxy* Proxy, 
                           vtkSMProperty* Property, 
                           QList<QVariant> Value)
{
  // TODO UNCHECKED

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
      Property->Modified();  
      // let ourselves know it was modified, since we blocked it previously
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
  
QVariant pqSMAdaptor::getEnumerationProperty(vtkSMProxy* Proxy, 
                                             vtkSMProperty* Property)
{
  Proxy->UpdatePropertyInformation(Property);
  QVariant var;

  vtkSMBooleanDomain* BooleanDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMProxyGroupDomain* ProxyGroupDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!BooleanDomain)
      {
      BooleanDomain = vtkSMBooleanDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!ProxyGroupDomain)
      {
      ProxyGroupDomain = vtkSMProxyGroupDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  vtkSMIntVectorProperty* ivp;
  vtkSMStringVectorProperty* svp;
  vtkSMProxyProperty* pp;
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);
  pp = vtkSMProxyProperty::SafeDownCast(Property);

  if(BooleanDomain && ivp && ivp->GetNumberOfElements() > 0)
    {
    var = (ivp->GetElement(0)) == 0 ? false : true;
    }
  else if(EnumerationDomain && ivp && ivp->GetNumberOfElements() > 0)
    {
    int val = ivp->GetElement(0);
    for (unsigned int i=0; i<EnumerationDomain->GetNumberOfEntries(); i++)
      {
      if (EnumerationDomain->GetEntryValue(i) == val)
        {
        var = EnumerationDomain->GetEntryText(i);
        break;
        }
      }
    }
  else if(StringListDomain && svp)
    {
    unsigned int nos = svp->GetNumberOfElements();
    for (unsigned int i=0; i < nos ; i++)
      {
      if (svp->GetElementType(i) == vtkSMStringVectorProperty::STRING)
        {
        var = svp->GetElement(i);
        break;
        }
      }
    }
  else if (ProxyGroupDomain && pp && pp->GetNumberOfProxies() > 0)
    {
    vtkSMProxy* p = pp->GetProxy(0);
    var = ProxyGroupDomain->GetProxyName(p);
    }

  return var;
}

void pqSMAdaptor::setEnumerationProperty(vtkSMProxy* Proxy, 
                                 vtkSMProperty* Property, QVariant Value)
{
  vtkSMBooleanDomain* BooleanDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMProxyGroupDomain* ProxyGroupDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!BooleanDomain)
      {
      BooleanDomain = vtkSMBooleanDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!ProxyGroupDomain)
      {
      ProxyGroupDomain = vtkSMProxyGroupDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  vtkSMIntVectorProperty* ivp;
  vtkSMStringVectorProperty* svp;
  vtkSMProxyProperty* pp;
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);
  pp = vtkSMProxyProperty::SafeDownCast(Property);

  if(BooleanDomain && ivp && ivp->GetNumberOfElements() > 0)
    {
    bool ok = true;
    int v = Value.toInt(&ok);
    if(ok)
      {
      ivp->SetElement(0, v);
      }
    }
  else if(EnumerationDomain && ivp && ivp->GetNumberOfElements() > 0)
    {
    QString str = Value.toString();
    unsigned int numEntries = EnumerationDomain->GetNumberOfEntries();
    for(unsigned int i=0; i<numEntries; i++)
      {
      if(str == EnumerationDomain->GetEntryText(i))
        {
        ivp->SetElement(0, EnumerationDomain->GetEntryValue(i));
        }
      }
    }
  else if(StringListDomain && svp)
    {
    unsigned int nos = svp->GetNumberOfElements();
    for (unsigned int i=0; i < nos ; i++)
      {
      if (svp->GetElementType(i) == vtkSMStringVectorProperty::STRING)
        {
        svp->SetElement(i, Value.toString().toAscii().data());
        }
      }
    }
  else if (ProxyGroupDomain && pp)
    {
    QString str = Value.toString();
    vtkSMProxy* toadd = ProxyGroupDomain->GetProxy(str.toAscii().data());
    if (pp->GetNumberOfProxies() < 1)
      {
      pp->AddProxy(toadd);
      }
    else
      {
      pp->SetProxy(0, toadd);
      }
    }

  Proxy->UpdateVTKObjects();
}

void pqSMAdaptor::setUncheckedEnumerationProperty(vtkSMProxy* vtkNotUsed(Proxy), 
                                 vtkSMProperty* Property, QVariant Value)
{
  vtkSMBooleanDomain* BooleanDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMProxyGroupDomain* ProxyGroupDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!BooleanDomain)
      {
      BooleanDomain = vtkSMBooleanDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!ProxyGroupDomain)
      {
      ProxyGroupDomain = vtkSMProxyGroupDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  vtkSMIntVectorProperty* ivp;
  vtkSMStringVectorProperty* svp;
  vtkSMProxyProperty* pp;
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);
  pp = vtkSMProxyProperty::SafeDownCast(Property);

  if(BooleanDomain && ivp && ivp->GetNumberOfElements() > 0)
    {
    bool ok = true;
    int v = Value.toInt(&ok);
    if(ok)
      {
      ivp->SetUncheckedElement(0, v);
      }
    }
  else if(EnumerationDomain && ivp && ivp->GetNumberOfElements() > 0)
    {
    QString str = Value.toString();
    unsigned int numEntries = EnumerationDomain->GetNumberOfEntries();
    for(unsigned int i=0; i<numEntries; i++)
      {
      if(str == EnumerationDomain->GetEntryText(i))
        {
        ivp->SetUncheckedElement(0, EnumerationDomain->GetEntryValue(i));
        }
      }
    }
  else if(StringListDomain && svp)
    {
    unsigned int nos = svp->GetNumberOfElements();
    for (unsigned int i=0; i < nos ; i++)
      {
      if (svp->GetElementType(i) == vtkSMStringVectorProperty::STRING)
        {
        svp->SetUncheckedElement(i, Value.toString().toAscii().data());
        }
      }
    }
  else if (ProxyGroupDomain && pp)
    {
    QString str = Value.toString();
    vtkSMProxy* toadd = ProxyGroupDomain->GetProxy(str.toAscii().data());
    if (pp->GetNumberOfUncheckedProxies() < 1)
      {
      pp->AddUncheckedProxy(toadd);
      }
    else
      {
      pp->SetUncheckedProxy(0, toadd);
      }
    }

  Property->UpdateDependentDomains();
}

QList<QVariant> pqSMAdaptor::getEnumerationPropertyDomain(
                                          vtkSMProperty* Property)
{
  Property->UpdateDependentDomains();
  QList<QVariant> enumerations;

  vtkSMBooleanDomain* BooleanDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMProxyGroupDomain* ProxyGroupDomain = NULL;
  vtkSMArrayListDomain* ArrayListDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!BooleanDomain)
      {
      BooleanDomain = vtkSMBooleanDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!ArrayListDomain)
      {
      ArrayListDomain = vtkSMArrayListDomain::SafeDownCast(d);
      }
    if(!ProxyGroupDomain)
      {
      ProxyGroupDomain = vtkSMProxyGroupDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();

  if(BooleanDomain)
    {
    enumerations.push_back(false);
    enumerations.push_back(true);
    }
  else if(ArrayListDomain)
    {
    unsigned int numEntries = ArrayListDomain->GetNumberOfStrings();
    for(unsigned int i=0; i<numEntries; i++)
      {
      enumerations.push_back(ArrayListDomain->GetString(i));
      }
    }
  else if(EnumerationDomain)
    {
    unsigned int numEntries = EnumerationDomain->GetNumberOfEntries();
    for(unsigned int i=0; i<numEntries; i++)
      {
      enumerations.push_back(EnumerationDomain->GetEntryText(i));
      }
    }
  else if(ProxyGroupDomain)
    {
    unsigned int numEntries = ProxyGroupDomain->GetNumberOfProxies();
    for(unsigned int i=0; i<numEntries; i++)
      {
      enumerations.push_back(ProxyGroupDomain->GetProxyName(i));
      }
    }
  else if(StringListDomain)
    {
    unsigned int numEntries = StringListDomain->GetNumberOfStrings();
    for(unsigned int i=0; i<numEntries; i++)
      {
      enumerations.push_back(StringListDomain->GetString(i));
      }
    }
  
  return enumerations;
}

QVariant pqSMAdaptor::getElementProperty(vtkSMProxy* Proxy, 
                                         vtkSMProperty* Property)
{
  return pqSMAdaptor::getMultipleElementProperty(Proxy, Property, 0);
}

void pqSMAdaptor::setElementProperty(vtkSMProxy* Proxy, 
                        vtkSMProperty* Property, QVariant Value)
{
  pqSMAdaptor::setMultipleElementProperty(Proxy, Property, 0, Value);
}

void pqSMAdaptor::setUncheckedElementProperty(vtkSMProxy* Proxy, 
                        vtkSMProperty* Property, QVariant Value)
{
  pqSMAdaptor::setUncheckedMultipleElementProperty(Proxy, Property, 0, Value);
}

QList<QVariant> pqSMAdaptor::getElementPropertyDomain(vtkSMProperty* Property)
{
  return pqSMAdaptor::getMultipleElementPropertyDomain(Property, 0);
}
  
QList<QVariant> pqSMAdaptor::getMultipleElementProperty(vtkSMProxy* Proxy, 
                                       vtkSMProperty* Property)
{
  Proxy->UpdatePropertyInformation(Property);
  QList<QVariant> props;
  
  vtkSMVectorProperty* VectorProperty;
  VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  if(!VectorProperty)
    {
    return props;
    }

  int i;
  int num = VectorProperty->GetNumberOfElements();
  for(i=0; i<num; i++)
    {
    props.push_back(
       pqSMAdaptor::getMultipleElementProperty(Proxy, Property, i)
       );
    }

  return props;
}

void pqSMAdaptor::setMultipleElementProperty(vtkSMProxy* Proxy, 
                      vtkSMProperty* Property, QList<QVariant> Value)
{
  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;
  vtkSMIdTypeVectorProperty* idvp;
  vtkSMStringVectorProperty* svp;
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(Property);
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  idvp = vtkSMIdTypeVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(dvp)
    {
    unsigned int num1 = Value.size();
    unsigned int num2 = dvp->GetNumberOfElements();
    unsigned int num = num1 < num2 ? num1 : num2;
    for(unsigned int i=0; i<num; i++)
      {
      bool ok = true;
      double v = Value[i].toDouble(&ok);
      if(ok)
        {
        dvp->SetElement(i, v);
        }
      }
    }
  else if(ivp)
    {
    unsigned int num1 = Value.size();
    unsigned int num2 = ivp->GetNumberOfElements();
    unsigned int num = num1 < num2 ? num1 : num2;
    for(unsigned int i=0; i<num; i++)
      {
      bool ok = true;
      int v = Value[i].toInt(&ok);
      if(ok)
        {
        ivp->SetElement(i, v);
        }
      }
    }
  else if(svp)
    {
    unsigned int num1 = Value.size();
    unsigned int num2 = svp->GetNumberOfElements();
    unsigned int num = num1 < num2 ? num1 : num2;
    for(unsigned int i=0; i<num; i++)
      {
      QString v = Value[i].toString();
      if(!v.isNull())
        {
        svp->SetElement(i, v.toAscii().data());
        }
      }
    }
  else if(idvp)
    {
    unsigned int num1 = Value.size();
    unsigned int num2 = idvp->GetNumberOfElements();
    unsigned int num = num1 < num2 ? num1 : num2;
    for(unsigned int i=0; i<num; i++)
      {
      bool ok = true;
      vtkIdType v;
#if defined(VTK_USE_64BIT_IDS)
      v = Value[i].toLongLong(&ok);
#else
      v = Value[i].toInt(&ok);
#endif
      if(ok)
        {
        idvp->SetElement(i, v);
        }
      }
    }
  Proxy->UpdateVTKObjects();
}

void pqSMAdaptor::setUncheckedMultipleElementProperty(vtkSMProxy* vtkNotUsed(Proxy), 
                      vtkSMProperty* Property, QList<QVariant> Value)
{
  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;
  vtkSMIdTypeVectorProperty* idvp;
  vtkSMStringVectorProperty* svp;
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(Property);
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  idvp = vtkSMIdTypeVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(dvp && dvp->GetNumberOfElements() >= (unsigned int)Value.size())
    {
    for(int i=0; i<Value.size(); i++)
      {
      bool ok = true;
      double v = Value[i].toDouble(&ok);
      if(ok)
        {
        dvp->SetUncheckedElement(i, v);
        }
      }
    }
  else if(ivp && ivp->GetNumberOfElements() >= (unsigned int)Value.size())
    {
    for(int i=0; i<Value.size(); i++)
      {
      bool ok = true;
      int v = Value[i].toInt(&ok);
      if(ok)
        {
        ivp->SetUncheckedElement(i, v);
        }
      }
    }
  else if(svp && svp->GetNumberOfElements() >= (unsigned int)Value.size())
    {
    for(int i=0; i<Value.size(); i++)
      {
      QString v = Value[i].toString();
      if(!v.isNull())
        {
        svp->SetUncheckedElement(i, v.toAscii().data());
        }
      }
    }
  else if(idvp && idvp->GetNumberOfElements() >= (unsigned int)Value.size())
    {
    for(int i=0; i<Value.size(); i++)
      {
      bool ok = true;
      vtkIdType v;
#if defined(VTK_USE_64BIT_IDS)
      v = Value[i].toLongLong(&ok);
#else
      v = Value[i].toInt(&ok);
#endif
      if(ok)
        {
        idvp->SetUncheckedElement(i, v);
        }
      }
    }
  Property->UpdateDependentDomains();
}

QList<QList<QVariant> > pqSMAdaptor::getMultipleElementPropertyDomain(
                           vtkSMProperty* Property)
{
  Property->UpdateDependentDomains();
  QList< QList<QVariant> > domains;
  
  vtkSMDoubleRangeDomain* DoubleDomain = NULL;
  vtkSMIntRangeDomain* IntDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!DoubleDomain)
      {
      DoubleDomain = vtkSMDoubleRangeDomain::SafeDownCast(d);
      }
    if(!IntDomain)
      {
      IntDomain = vtkSMIntRangeDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();

  if(DoubleDomain)
    {
    vtkSMDoubleVectorProperty* dvp;
    dvp = vtkSMDoubleVectorProperty::SafeDownCast(Property);

    unsigned int numElems = dvp->GetNumberOfElements();
    for(unsigned int i=0; i<numElems; i++)
      {
      QList<QVariant> domain;
      int exists1, exists2;
      double min = DoubleDomain->GetMinimum(i, exists1);
      double max = DoubleDomain->GetMaximum(i, exists2);
      if(exists1 && exists2)  // what if one of them exists?
        {
        domain.push_back(min);
        domain.push_back(max);
        }
      domains.push_back(domain);
      }
    }
  else if(IntDomain)
    {
    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(Property);

    unsigned int numElems = ivp->GetNumberOfElements();
    for(unsigned int i=0; i<numElems; i++)
      {
      QList<QVariant> domain;
      int exists1, exists2;
      int min = IntDomain->GetMinimum(i, exists1);
      int max = IntDomain->GetMaximum(i, exists2);
      if(exists1 && exists2)  // what if one of them exists?
        {
        domain.push_back(min);
        domain.push_back(max);
        }
      domains.push_back(domain);
      }
    }

  return domains;
}

QVariant pqSMAdaptor::getMultipleElementProperty(vtkSMProxy* Proxy, 
                               vtkSMProperty* Property, unsigned int Index)
{
  Proxy->UpdatePropertyInformation(Property);
  QVariant var;
  
  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;
  vtkSMIdTypeVectorProperty* idvp;
  vtkSMStringVectorProperty* svp;
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(Property);
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  idvp = vtkSMIdTypeVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(dvp && dvp->GetNumberOfElements() > Index)
    {
    var = dvp->GetElement(Index);
    }
  else if(ivp && ivp->GetNumberOfElements() > Index)
    {
    var = ivp->GetElement(Index);
    }
  else if(svp && svp->GetNumberOfElements() > Index)
    {
    var = svp->GetElement(Index);
    }
  else if(idvp && idvp->GetNumberOfElements() > Index)
    {
    var = idvp->GetElement(Index);
    }
  
  return var;
}

void pqSMAdaptor::setMultipleElementProperty(vtkSMProxy* Proxy, 
                    vtkSMProperty* Property, unsigned int Index, QVariant Value)
{
  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;
  vtkSMIdTypeVectorProperty* idvp;
  vtkSMStringVectorProperty* svp;
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(Property);
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  idvp = vtkSMIdTypeVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(dvp && dvp->GetNumberOfElements() > Index)
    {
    bool ok = true;
    double v = Value.toDouble(&ok);
    if(ok)
      {
      dvp->SetElement(Index, v);
      }
    }
  else if(ivp && ivp->GetNumberOfElements() > Index)
    {
    bool ok = true;
    int v = Value.toInt(&ok);
    if(ok)
      {
      ivp->SetElement(Index, v);
      }
    }
  else if(svp && svp->GetNumberOfElements() > Index)
    {
    QString v = Value.toString();
    if(!v.isNull())
      {
      svp->SetElement(Index, v.toAscii().data());
      }
    }
  else if(idvp && idvp->GetNumberOfElements() > Index)
    {
    bool ok = true;
    vtkIdType v;
#if defined(VTK_USE_64BIT_IDS)
    v = Value.toLongLong(&ok);
#else
    v = Value.toInt(&ok);
#endif
    if(ok)
      {
      idvp->SetElement(Index, v);
      }
    }
  Proxy->UpdateVTKObjects();
}

void pqSMAdaptor::setUncheckedMultipleElementProperty(vtkSMProxy* vtkNotUsed(Proxy), 
                    vtkSMProperty* Property, unsigned int Index, QVariant Value)
{
  vtkSMDoubleVectorProperty* dvp;
  vtkSMIntVectorProperty* ivp;
  vtkSMIdTypeVectorProperty* idvp;
  vtkSMStringVectorProperty* svp;
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(Property);
  ivp = vtkSMIntVectorProperty::SafeDownCast(Property);
  idvp = vtkSMIdTypeVectorProperty::SafeDownCast(Property);
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(dvp && dvp->GetNumberOfElements() > Index)
    {
    bool ok = true;
    double v = Value.toDouble(&ok);
    if(ok)
      {
      dvp->SetUncheckedElement(Index, v);
      }
    }
  else if(ivp && ivp->GetNumberOfElements() > Index)
    {
    bool ok = true;
    int v = Value.toInt(&ok);
    if(ok)
      {
      ivp->SetUncheckedElement(Index, v);
      }
    }
  else if(svp && svp->GetNumberOfElements() > Index)
    {
    QString v = Value.toString();
    if(!v.isNull())
      {
      svp->SetUncheckedElement(Index, v.toAscii().data());
      }
    }
  else if(idvp && idvp->GetNumberOfElements() > Index)
    {
    bool ok = true;
    vtkIdType v;
#if defined(VTK_USE_64BIT_IDS)
    v = Value.toLongLong(&ok);
#else
    v = Value.toInt(&ok);
#endif
    if(ok)
      {
      idvp->SetUncheckedElement(Index, v);
      }
    }
  Property->UpdateDependentDomains();
}

QList<QVariant> pqSMAdaptor::getMultipleElementPropertyDomain(
                        vtkSMProperty* Property, unsigned int Index)
{
  Property->UpdateDependentDomains();
  QList<QVariant> domain;
  
  vtkSMDoubleRangeDomain* DoubleDomain = NULL;
  vtkSMIntRangeDomain* IntDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!DoubleDomain)
      {
      DoubleDomain = vtkSMDoubleRangeDomain::SafeDownCast(d);
      }
    if(!IntDomain)
      {
      IntDomain = vtkSMIntRangeDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();

  if(DoubleDomain)
    {
    int exists1, exists2;
    double min = DoubleDomain->GetMinimum(Index, exists1);
    double max = DoubleDomain->GetMaximum(Index, exists2);
    if(exists1 && exists2)  // what if one of them exists?
      {
      domain.push_back(min);
      domain.push_back(max);
      }
    }
  else if(IntDomain)
    {
    int exists1, exists2;
    int min = IntDomain->GetMinimum(Index, exists1);
    int max = IntDomain->GetMaximum(Index, exists2);
    if(exists1 && exists2)  // what if one of them exists?
      {
      domain.push_back(min);
      domain.push_back(max);
      }
    }

  return domain;
}

QString pqSMAdaptor::getFileListProperty(vtkSMProxy* Proxy, 
                               vtkSMProperty* Property)
{
  Proxy->UpdatePropertyInformation(Property);
  QString file;
  
  vtkSMStringVectorProperty* svp;
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(svp && svp->GetNumberOfElements() > 0)
    {
    file = svp->GetElement(0);
    }
  return file;
}

void pqSMAdaptor::setFileListProperty(vtkSMProxy* Proxy, 
                            vtkSMProperty* Property, QString Value)
{
  vtkSMStringVectorProperty* svp;
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(svp && svp->GetNumberOfElements() > 0)
    {
    if(!Value.isNull())
      {
      svp->SetElement(0, Value.toAscii().data());
      }
    }
  Proxy->UpdateVTKObjects();
}

void pqSMAdaptor::setUncheckedFileListProperty(vtkSMProxy* vtkNotUsed(Proxy), 
                            vtkSMProperty* Property, QString Value)
{
  vtkSMStringVectorProperty* svp;
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(svp && svp->GetNumberOfElements() > 0)
    {
    if(!Value.isNull())
      {
      svp->SetUncheckedElement(0, Value.toAscii().data());
      }
    }
  Property->UpdateDependentDomains();
}

