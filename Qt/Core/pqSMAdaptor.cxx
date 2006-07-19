/*=========================================================================

   Program: ParaView
   Module:    pqSMAdaptor.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "vtkSMArrayListDomain.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringListRangeDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMVectorProperty.h"

// ParaView includes
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
    if (vtkSMProxyListDomain::SafeDownCast(Property->GetDomain("proxy_list")))
      {
      type = pqSMAdaptor::PROXYSELECTION;
      }
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

pqSMProxy pqSMAdaptor::getProxyProperty(vtkSMProperty* Property)
{
  pqSMAdaptor::PropertyType type = pqSMAdaptor::getPropertyType(Property);
  if( type == pqSMAdaptor::PROXY || type == pqSMAdaptor::PROXYSELECTION)
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
      domain = pqSMAdaptor::getProxyPropertyDomain(Property);
      if(domain.size())
        {
        //pqSMAdaptor::setProxyProperty(Property, domain[0]);
        return domain[0];
        }
      }
    }
  return pqSMProxy(NULL);
}

void pqSMAdaptor::setProxyProperty(vtkSMProperty* Property, 
                                   pqSMProxy Value)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    if (proxyProp->GetNumberOfProxies() == 1)
      {
      proxyProp->SetProxy(0, Value);
      }
    else
      {
      proxyProp->RemoveAllProxies();
      proxyProp->AddProxy(Value);
      }
    }
}

void pqSMAdaptor::setUncheckedProxyProperty(vtkSMProperty* Property,
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

QList<pqSMProxy> pqSMAdaptor::getProxyListProperty(vtkSMProperty* Property)
{
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

void pqSMAdaptor::setProxyListProperty(vtkSMProperty* Property, 
                                       QList<pqSMProxy> Value)
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
}

QList<pqSMProxy> pqSMAdaptor::getProxyPropertyDomain(vtkSMProperty* Property)
{
  QList<pqSMProxy> proxydomain;
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    vtkSMProxyManager* pm = vtkSMProxyManager::GetProxyManager();
    
    // get group domain of this property 
    // and add all proxies in those groups to our list
    vtkSMProxyGroupDomain* gd;
    vtkSMProxyListDomain* ld;
    ld = vtkSMProxyListDomain::SafeDownCast(Property->GetDomain("proxy_list"));
    gd = vtkSMProxyGroupDomain::SafeDownCast(Property->GetDomain("groups"));
    if (ld)
      {
      unsigned int numProxies = ld->GetNumberOfProxies();
      for (unsigned int cc=0; cc < numProxies; cc++)
        {
        proxydomain.append(ld->GetProxy(cc));
        }
      }
    else if (gd)
      {
      unsigned int numGroups = gd->GetNumberOfGroups();
      for(unsigned int i=0; i<numGroups; i++)
        {
        const char* group = gd->GetGroup(i);
        unsigned int numProxies = pm->GetNumberOfProxies(group);
        for(unsigned int j=0; j<numProxies; j++)
          {
          pqSMProxy p = pm->GetProxy(group, pm->GetProxyName(group, j));
          proxydomain.append(p);
          }
        }
      }
    }
  return proxydomain;
}


QList<QList<QVariant> > pqSMAdaptor::getSelectionProperty(vtkSMProperty* Property)
{
  QList<QList<QVariant> > ret;
  
  vtkSMStringVectorProperty* StringProperty = NULL;
  StringProperty = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(StringProperty)
    {
    int numSelections = 0;
    vtkSMStringVectorProperty* infoProp = NULL;
    infoProp = vtkSMStringVectorProperty::SafeDownCast(
                           StringProperty->GetInformationProperty());
    if(infoProp)
      {
      numSelections = infoProp->GetNumberOfElements() / 2;
      }
    else
      {
      numSelections = StringProperty->GetNumberOfElements() / 2;
      }

    for(int i=0; i<numSelections; i++)
      {
      QList<QVariant> tmp;
      tmp = pqSMAdaptor::getSelectionProperty(Property, i);
      ret.append(tmp);
      }
    }

  return ret;
}

QList<QVariant> pqSMAdaptor::getSelectionProperty(vtkSMProperty* Property, 
                                                  unsigned int Index)
{
  QList<QVariant> ret;
  vtkSMStringVectorProperty* StringProperty = NULL;
  StringProperty = vtkSMStringVectorProperty::SafeDownCast(Property);
  vtkSMStringListRangeDomain* StringDomain = NULL;

  if(StringProperty)
    {
    vtkSMDomainIterator* iter = Property->NewDomainIterator();
    iter->Begin();
    while(StringDomain == NULL && !iter->IsAtEnd())
      {
      vtkSMDomain* d = iter->GetDomain();
      StringDomain = vtkSMStringListRangeDomain::SafeDownCast(d);
      iter->Next();
      }
    iter->Delete();
    }
  
  if(StringProperty && StringDomain)
    {
    QString StringName = StringDomain->GetString(Index);
    if(!StringName.isNull())
      {
      ret.append(StringName);
      QVariant value;

      int numElements = StringProperty->GetNumberOfElements();
      for(int i=0; i<numElements; i+=2)
        {
        if(StringName == StringProperty->GetElement(i))
          {
          value = StringProperty->GetElement(i+1);
          break;
          }
        }
      // check the information property for a default value
      if(!value.isValid())
        {
        vtkSMStringVectorProperty* infoProp = NULL;
        infoProp = vtkSMStringVectorProperty::SafeDownCast(
                               StringProperty->GetInformationProperty());
        if(infoProp)
          {
          value = infoProp->GetElement(Index*2 + 1);
          }
        }
      // make up a zero
      if(!value.isValid())
        {
        value = "0";
        }
      if(StringDomain->GetIntDomainMode() ==
         vtkSMStringListRangeDomain::BOOLEAN)
        {
        value.convert(QVariant::Bool);
        }
      ret.append(value);
      }
    }

  return ret;
}

void pqSMAdaptor::setSelectionProperty(vtkSMProperty* Property, 
                                   QList<QList<QVariant> > Value)
{
  foreach(QList<QVariant> l, Value)
    {
    pqSMAdaptor::setSelectionProperty(Property, l);
    }
}

void pqSMAdaptor::setUncheckedSelectionProperty(vtkSMProperty* Property,
                                  QList<QList<QVariant> > Value)
{
  foreach(QList<QVariant> l, Value)
    {
    pqSMAdaptor::setUncheckedSelectionProperty(Property, l);
    }
}

// TODO:  callers need to clear items from vtkStringVectorProperty before
//        settings all the values to push down to the server
void pqSMAdaptor::setSelectionProperty(vtkSMProperty* Property, 
                                       QList<QVariant> Value)
{
  vtkSMStringVectorProperty* StringProperty;
  StringProperty = vtkSMStringVectorProperty::SafeDownCast(Property);
  if(StringProperty && Value.size() == 2)
    {
    vtkSMStringListRangeDomain* StringDomain = NULL;
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
      QString name = Value[0].toString();
      QVariant value = Value[1];
      if(value.type() == QVariant::Bool)
        {
        value = value.toInt();
        }
      QString valueStr = value.toString();
      unsigned int numElems;
      numElems = StringProperty->GetNumberOfElements();
      if (numElems % 2 != 0)
        {
        return;
        }
      unsigned int i;
      for(i=0; i<numElems; i+=2)
        {
        if(name == StringProperty->GetElement(i))
          {
          StringProperty->SetElement(i+1, valueStr.toAscii().data());
          return;
          }
        }
      // not found, just put it in the first empty slot
      for(i=0; i<numElems; i+=2)
        {
        const char* elem = StringProperty->GetElement(i);
        if(!elem || elem[0] == '\0')
          {
          StringProperty->SetElement(i, name.toAscii().data());
          StringProperty->SetElement(i+1, valueStr.toAscii().data());
          return;
          }
        }
      // If we didn't find any empty spots, append to the vector
      StringProperty->SetElement(numElems, name.toAscii().data());
      StringProperty->SetElement(numElems+1, valueStr.toAscii().data());
      return;
      }
    }
}

void pqSMAdaptor::setUncheckedSelectionProperty(vtkSMProperty* Property,
                                                QList<QVariant> Value)
{
  vtkSMStringVectorProperty* StringProperty;
  StringProperty = vtkSMStringVectorProperty::SafeDownCast(Property);
  if(StringProperty && Value.size() == 2)
    {
    vtkSMStringListRangeDomain* StringDomain = NULL;
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
      QString name = Value[0].toString();
      QVariant value = Value[1];
      if(value.type() == QVariant::Bool)
        {
        value = value.toInt();
        }
      QString valueStr = value.toString();
      unsigned int numElems;
      numElems = StringProperty->GetNumberOfUncheckedElements();
      if (numElems % 2 != 0)
        {
        return;
        }
      unsigned int i;
      for(i=0; i<numElems; i+=2)
        {
        if(name == StringProperty->GetUncheckedElement(i))
          {
          StringProperty->SetUncheckedElement(i+1, valueStr.toAscii().data());
          Property->UpdateDependentDomains();
          return;
          }
        }
      // not found, just put it in the first empty slot
      for(i=0; i<numElems; i+=2)
        {
        const char* elem = StringProperty->GetUncheckedElement(i);
        if(!elem || elem[0] == '\0')
          {
          StringProperty->SetUncheckedElement(i, name.toAscii().data());
          StringProperty->SetUncheckedElement(i+1, valueStr.toAscii().data());
          Property->UpdateDependentDomains();
          return;
          }
        }
      // If we didn't find any empty spots, append to the vector
      StringProperty->SetUncheckedElement(numElems, name.toAscii().data());
      StringProperty->SetUncheckedElement(numElems+1, valueStr.toAscii().data());
      Property->UpdateDependentDomains();
      return;
      }
    }
}

QList<QVariant> pqSMAdaptor::getSelectionPropertyDomain(vtkSMProperty* Property)
{
  QList<QVariant> ret;
  
  vtkSMStringVectorProperty* StringProperty;
  StringProperty = vtkSMStringVectorProperty::SafeDownCast(Property);
  if(StringProperty)
    {
    vtkSMStringListRangeDomain* StringDomain = NULL;
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
      int num = StringDomain->GetNumberOfStrings();
      for(int i=0; i<num; i++)
        {
        ret.append(StringDomain->GetString(i));
        }
      }
    }
  return ret;
}
  
QVariant pqSMAdaptor::getEnumerationProperty(vtkSMProperty* Property)
{
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

void pqSMAdaptor::setEnumerationProperty(vtkSMProperty* Property,
                                         QVariant Value)
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
}

void pqSMAdaptor::setUncheckedEnumerationProperty(vtkSMProperty* Property,
                                                  QVariant Value)
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
      Property->UpdateDependentDomains();
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
        Property->UpdateDependentDomains();
        }
      }
    }
  else if(StringListDomain && svp)
    {
    QString str = Value.toString();
    unsigned int nos = svp->GetNumberOfElements();
    for (unsigned int i=0; i < nos ; i++)
      {
      if (svp->GetElementType(i) == vtkSMStringVectorProperty::STRING)
        {
        svp->SetUncheckedElement(i, str.toAscii().data());
        Property->UpdateDependentDomains();
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
      Property->UpdateDependentDomains();
      }
    else
      {
      pp->SetUncheckedProxy(0, toadd);
      Property->UpdateDependentDomains();
      }
    }

}

QList<QVariant> pqSMAdaptor::getEnumerationPropertyDomain(
                                          vtkSMProperty* Property)
{
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

QVariant pqSMAdaptor::getElementProperty(vtkSMProperty* Property)
{
  return pqSMAdaptor::getMultipleElementProperty(Property, 0);
}

void pqSMAdaptor::setElementProperty(vtkSMProperty* Property, QVariant Value)
{
  pqSMAdaptor::setMultipleElementProperty(Property, 0, Value);
}

void pqSMAdaptor::setUncheckedElementProperty(vtkSMProperty* Property, 
                                              QVariant Value)
{
  pqSMAdaptor::setUncheckedMultipleElementProperty(Property, 0, Value);
}

QList<QVariant> pqSMAdaptor::getElementPropertyDomain(vtkSMProperty* Property)
{
  return pqSMAdaptor::getMultipleElementPropertyDomain(Property, 0);
}
  
QList<QVariant> pqSMAdaptor::getMultipleElementProperty(vtkSMProperty* Property)
{
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
       pqSMAdaptor::getMultipleElementProperty(Property, i)
       );
    }

  return props;
}

void pqSMAdaptor::setMultipleElementProperty(vtkSMProperty* Property, 
                                             QList<QVariant> Value)
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
}

void pqSMAdaptor::setUncheckedMultipleElementProperty(vtkSMProperty* Property,
                                                      QList<QVariant> Value)
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

QVariant pqSMAdaptor::getMultipleElementProperty(vtkSMProperty* Property,
                                                 unsigned int Index)
{
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

void pqSMAdaptor::setMultipleElementProperty(vtkSMProperty* Property, 
                                             unsigned int Index,
                                             QVariant Value)
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
}

void pqSMAdaptor::setUncheckedMultipleElementProperty(vtkSMProperty* Property, 
                                     unsigned int Index, QVariant Value)
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

QString pqSMAdaptor::getFileListProperty(vtkSMProperty* Property)
{
  QString file;
  
  vtkSMStringVectorProperty* svp;
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if(svp && svp->GetNumberOfElements() > 0)
    {
    file = svp->GetElement(0);
    }
  return file;
}

void pqSMAdaptor::setFileListProperty(vtkSMProperty* Property, QString Value)
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
}

void pqSMAdaptor::setUncheckedFileListProperty(vtkSMProperty* Property,
                                               QString Value)
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

