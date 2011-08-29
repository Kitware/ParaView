/*=========================================================================

   Program: ParaView
   Module:    pqSMAdaptor.cxx

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

// self includes
#include "pqSMAdaptor.h"

// qt includes
#include <QString>
#include <QVariant>

// vtk includes
#include "vtkConfigure.h"   // for 64-bitness
#include "vtkStringList.h"
#include "vtkSmartPointer.h"

// server manager includes
#include "vtkSMArrayListDomain.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
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
#include "vtkSMExtentDomain.h"
#include "vtkSMFileListDomain.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMSILDomain.h"

// ParaView includes
#include "pqSMProxy.h"

#include <QStringList>

static const int metaId = qRegisterMetaType<QList<QList<QVariant> > >("ListOfList");

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
  vtkSMVectorProperty* VectorProperty = 
    vtkSMVectorProperty::SafeDownCast(Property);
  
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
  else if(Property->GetDomain("field_list"))
    {
    type = pqSMAdaptor::FIELD_SELECTION;
    }
  else
    {
    vtkSMStringListRangeDomain* stringListRangeDomain = NULL;
    vtkSMBooleanDomain* booleanDomain = NULL;
    vtkSMEnumerationDomain* enumerationDomain = NULL;
    vtkSMProxyGroupDomain* proxyGroupDomain = NULL;
    vtkSMFileListDomain* fileListDomain = NULL;
    vtkSMStringListDomain* stringListDomain = NULL;
    vtkSMCompositeTreeDomain* compositeTreeDomain = NULL;
    vtkSMSILDomain* silDomain = NULL;
    
    vtkSMDomainIterator* iter = Property->NewDomainIterator();
    for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      if (!silDomain)
        {
        silDomain = vtkSMSILDomain::SafeDownCast(iter->GetDomain());
        }
      if(!stringListRangeDomain)
        {
        stringListRangeDomain = vtkSMStringListRangeDomain::SafeDownCast(iter->GetDomain());
        }
      if(!booleanDomain)
        {
        booleanDomain = vtkSMBooleanDomain::SafeDownCast(iter->GetDomain());
        }
      if(!enumerationDomain)
        {
        enumerationDomain = vtkSMEnumerationDomain::SafeDownCast(iter->GetDomain());
        }
      if(!proxyGroupDomain)
        {
        proxyGroupDomain = vtkSMProxyGroupDomain::SafeDownCast(iter->GetDomain());
        }
      if(!fileListDomain)
        {
        fileListDomain = vtkSMFileListDomain::SafeDownCast(iter->GetDomain());
        }
      if(!stringListDomain)
        {
        stringListDomain = vtkSMStringListDomain::SafeDownCast(iter->GetDomain());
        }
      if (!compositeTreeDomain)
        {
        compositeTreeDomain = vtkSMCompositeTreeDomain::SafeDownCast(iter->GetDomain());
        }
      }
    iter->Delete();

    if(fileListDomain)
      {
      type = pqSMAdaptor::FILE_LIST;
      }
    else if (compositeTreeDomain)
      {
      type = pqSMAdaptor::COMPOSITE_TREE;
      }
    else if (silDomain)
      {
      type = pqSMAdaptor::SIL;
      }
    else if(!silDomain && (
      stringListRangeDomain || 
      (VectorProperty && VectorProperty->GetRepeatCommand() && 
       (stringListDomain || enumerationDomain))))
      {
      type = pqSMAdaptor::SELECTION;
      }
    else if(booleanDomain || enumerationDomain || 
            proxyGroupDomain || stringListDomain)
      {
      type = pqSMAdaptor::ENUMERATION;
      }
    else 
      {
      if(VectorProperty && 
        (VectorProperty->GetNumberOfElements() > 1 || VectorProperty->GetRepeatCommand()))
        {
        type = pqSMAdaptor::MULTIPLE_ELEMENTS;
        }
      else if(VectorProperty && VectorProperty->GetNumberOfElements() == 1)
        {
        type = pqSMAdaptor::SINGLE_ELEMENT;
        }
      }
    }

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
    }
  return pqSMProxy(NULL);
}

void pqSMAdaptor::removeProxyProperty(vtkSMProperty* Property, pqSMProxy Value)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    proxyProp->RemoveProxy(Value);
    }
}

//-----------------------------------------------------------------------------
void pqSMAdaptor::addInputProperty(vtkSMProperty* Property, 
                               pqSMProxy Value, int opport)
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(Property);
  if (ip)
    {
    ip->AddInputConnection(Value, opport);
    }
}

//-----------------------------------------------------------------------------
void pqSMAdaptor::setInputProperty(vtkSMProperty* Property, 
                                   pqSMProxy Value, int opport)
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(Property);
  if (ip)
    {
    if (ip->GetNumberOfProxies() == 1)
      {
      ip->SetInputConnection(0, Value, opport);
      }
    else
      {
      ip->RemoveAllProxies();
      ip->AddInputConnection(Value, opport);
      }
    }
}

//-----------------------------------------------------------------------------
void pqSMAdaptor::addProxyProperty(vtkSMProperty* Property, 
                                   pqSMProxy Value)
{
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if(proxyProp)
    {
    proxyProp->AddProxy(Value);
    }
}

//-----------------------------------------------------------------------------
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
  vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(Property);
  if (proxyProp)
    {
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
    vtkSMProxyManager* pm = Property->GetParent()->GetProxyManager();
    
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

  if(!Property)
    {
    return ret;
    }
  
  vtkSMStringListRangeDomain* StringDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringDomain)
      {
      StringDomain = vtkSMStringListRangeDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  int numSelections = 0;
  if(EnumerationDomain)
    {
    numSelections = EnumerationDomain->GetNumberOfEntries();
    }
  else if(StringListDomain)
    {
    numSelections = StringListDomain->GetNumberOfStrings();
    }
  else if(StringDomain)
    {
    numSelections = StringDomain->GetNumberOfStrings();
    }

  for(int i=0; i<numSelections; i++)
    {
    QList<QVariant> tmp;
    tmp = pqSMAdaptor::getSelectionProperty(Property, i);
    ret.append(tmp);
    }

  return ret;
}

QList<QVariant> pqSMAdaptor::getSelectionProperty(vtkSMProperty* Property, 
                                                  unsigned int Index)
{
  QList<QVariant> ret;
  
  if(!Property)
    {
    return ret;
    }
  
  vtkSMStringListRangeDomain* StringDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringDomain)
      {
      StringDomain = vtkSMStringListRangeDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  vtkSMStringVectorProperty* StringProperty = NULL;
  StringProperty = vtkSMStringVectorProperty::SafeDownCast(Property);
  if(StringProperty && StringDomain)
    {
    QString StringName = StringDomain->GetString(Index);
    if(!StringName.isNull())
      {
      ret.append(StringName);
      QVariant value;

      int numElements = StringProperty->GetNumberOfElements();
      if(numElements % 2 == 0)
        {
        for(int i=0; i<numElements; i+=2)
          {
          if(StringName == StringProperty->GetElement(i))
            {
            value = StringProperty->GetElement(i+1);
            break;
            }
          }
        }

      vtkSMStringVectorProperty* infoSP = vtkSMStringVectorProperty::SafeDownCast(
        StringProperty->GetInformationProperty());
      if (!value.isValid() && infoSP)
        {
        // check if the information property is giving us the status for the
        // array selection.

        numElements = infoSP->GetNumberOfElements();
        for(int i=0; (i+1)<numElements; i+=2)
          {
          if(StringName == infoSP->GetElement(i))
            {
            value = infoSP->GetElement(i+1);
            break;
            }
          }
        }

      // make up a zero
      if(!value.isValid())
        {
        qWarning("had to make up a value for selection\n");
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
  else if(StringListDomain)
    {
    QList<QVariant> values =
      pqSMAdaptor::getMultipleElementProperty(Property);
    
    if(Index < StringListDomain->GetNumberOfStrings())
      {
      QVariant whichDomain = StringListDomain->GetString(Index);
      ret.append(whichDomain);
      if(values.contains(whichDomain))
        {
        ret.append(1);
        }
      else
        {
        ret.append(0);
        }
      }
    else
      {
      qWarning("index out of range for arraylist domain\n");
      }
    }
  else if(EnumerationDomain)
    {
    QList<QVariant> values =
      pqSMAdaptor::getMultipleElementProperty(Property);

    if(Index < EnumerationDomain->GetNumberOfEntries())
      {
      ret.append(EnumerationDomain->GetEntryText(Index));
      if(values.contains(EnumerationDomain->GetEntryValue(Index)))
        {
        ret.append(1);
        }
      else
        {
        ret.append(0);
        }
      }
    else
      {
      qWarning("index out of range for enumeration domain\n");
      }
    }

  return ret;
}

void pqSMAdaptor::setSelectionProperty(vtkSMProperty* Property, 
                                   QList<QList<QVariant> > Value)
{
  if(!Property)
    {
    return;
    }

  vtkSMVectorProperty* VectorProperty;
  VectorProperty = vtkSMVectorProperty::SafeDownCast(Property);
  
  vtkSMStringListRangeDomain* StringDomain = NULL;

  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringDomain)
      {
      StringDomain = vtkSMStringListRangeDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();

  VectorProperty->SetNumberOfElements(0);
  
  foreach(QList<QVariant> l, Value)
    {
    pqSMAdaptor::setSelectionProperty(Property, l);
    }
}

void pqSMAdaptor::setUncheckedSelectionProperty(vtkSMProperty* Property,
                                  QList<QList<QVariant> > Value)
{
  if(!Property)
    {
    return;
    }

  foreach(QList<QVariant> l, Value)
    {
    pqSMAdaptor::setUncheckedSelectionProperty(Property, l);
    }
}

void pqSMAdaptor::setSelectionProperty(vtkSMProperty* Property, 
                                       QList<QVariant> Value)
{

  if(!Property || Value.size() != 2)
    {
    return;
    }

  vtkSMVectorProperty* VectorProperty =
    vtkSMVectorProperty::SafeDownCast(Property);
  
  vtkSMStringListRangeDomain* StringDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;

  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringDomain)
      {
      StringDomain = vtkSMStringListRangeDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  vtkSMStringVectorProperty* StringProperty;
  StringProperty = vtkSMStringVectorProperty::SafeDownCast(Property);
  if(StringProperty && StringDomain)
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

    // Not found, add it...
    // We will create a vtkStringList for the name,value pair and then
    // set the string values in one atomic call.
    vtkSmartPointer<vtkStringList> stringValues = vtkSmartPointer<vtkStringList>::New();
    StringProperty->GetElements(stringValues);
    numElems = stringValues->GetLength();

    // First look for an empty slot to add the values
    for(i=0; i<numElems; i+=2)
      {
      const char* elem = StringProperty->GetElement(i);
      if(!elem || elem[0] == '\0')
        {
        stringValues->SetString(i, name.toAscii().data());
        stringValues->SetString(i+1, valueStr.toAscii().data());
        StringProperty->SetElements(stringValues);
        return;
        }
      }

    // Add the values at the end
    stringValues->SetString(numElems, name.toAscii().data());
    stringValues->SetString(numElems+1, valueStr.toAscii().data());
    StringProperty->SetElements(stringValues);
    }
  else if(EnumerationDomain)
    {
    QList<QVariant> domainStrings =
      pqSMAdaptor::getEnumerationPropertyDomain(Property);
    int idx = domainStrings.indexOf(Value[0]);
    if(Value[1].toInt() && idx != -1)
      {
      pqSMAdaptor::setMultipleElementProperty(VectorProperty,
                                              VectorProperty->GetNumberOfElements(),
                                              EnumerationDomain->GetEntryValue(idx));
      }
    }
  else if(StringListDomain)
    {
    QList<QVariant> values =
      pqSMAdaptor::getMultipleElementProperty(Property);
    if(Value[1].toInt() && !values.contains(Value[0]))
      {
      pqSMAdaptor::setMultipleElementProperty(Property, values.size(),
                                              Value[0]);
      }
    }
}

void pqSMAdaptor::setUncheckedSelectionProperty(vtkSMProperty* Property,
                                                QList<QVariant> Value)
{
  if(!Property || Value.size() != 2)
    {
    return;
    }

  vtkSMVectorProperty* VectorProperty =
    vtkSMVectorProperty::SafeDownCast(Property);
  
  vtkSMStringListRangeDomain* StringDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;

  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringDomain)
      {
      StringDomain = vtkSMStringListRangeDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
    iter->Next();
    }
  iter->Delete();
  
  vtkSMStringVectorProperty* StringProperty;
  StringProperty = vtkSMStringVectorProperty::SafeDownCast(VectorProperty);
  if(StringProperty && StringDomain)
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
    }
  else if(EnumerationDomain)
    {
    QList<QVariant> domainStrings =
      pqSMAdaptor::getEnumerationPropertyDomain(VectorProperty);
    int idx = domainStrings.indexOf(Value[0]);
    if(Value[1].toInt() && idx != -1)
      {
      pqSMAdaptor::setUncheckedMultipleElementProperty(Property,
                                              VectorProperty->GetNumberOfElements(),
                                              EnumerationDomain->GetEntryValue(idx));
      }
    }
  else if(StringListDomain)
    {
    QList<QVariant> values =
      pqSMAdaptor::getMultipleElementProperty(Property);
    if(Value[1].toInt() && !values.contains(Value[0]))
      {
      pqSMAdaptor::setUncheckedMultipleElementProperty(Property, values.size(),
                                              Value[0]);
      }
    }
}

QList<QVariant> pqSMAdaptor::getSelectionPropertyDomain(vtkSMProperty* Property)
{
  QList<QVariant> ret;
  if(!Property)
    {
    return ret;
    }

  vtkSMVectorProperty* VProperty = vtkSMVectorProperty::SafeDownCast(Property);

  vtkSMStringListRangeDomain* StringDomain = NULL;
  vtkSMStringListDomain* StringListDomain = NULL;
  vtkSMEnumerationDomain* EnumerationDomain = NULL;

  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd())
    {
    vtkSMDomain* d = iter->GetDomain();
    if(!StringDomain)
      {
      StringDomain = vtkSMStringListRangeDomain::SafeDownCast(d);
      }
    if(!StringListDomain)
      {
      StringListDomain = vtkSMStringListDomain::SafeDownCast(d);
      }
    if(!EnumerationDomain)
      {
      EnumerationDomain = vtkSMEnumerationDomain::SafeDownCast(d);
      }
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
  else if(EnumerationDomain && VProperty->GetRepeatCommand())
    {
    unsigned int numEntries = EnumerationDomain->GetNumberOfEntries();
    for(unsigned int i=0; i<numEntries; i++)
      {
      ret.append(EnumerationDomain->GetEntryText(i));
      }
    }
  else if(StringListDomain && VProperty->GetRepeatCommand())
    {
    unsigned int numEntries = StringListDomain->GetNumberOfStrings();
    for(unsigned int i=0; i<numEntries; i++)
      {
      ret.append(StringListDomain->GetString(i));
      }
    }
  
  return ret;
}
  
QVariant pqSMAdaptor::getEnumerationProperty(vtkSMProperty* Property)
{
  QVariant var;
  if(!Property)
    {
    return var;
    }

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
  if(!Property)
    {
    return;
    }

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
  else if(EnumerationDomain && ivp)
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
  if(!Property)
    {
    return;
    }

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
    unsigned int nos = svp->GetNumberOfElements();
    for (unsigned int i=0; i < nos ; i++)
      {
      if (svp->GetElementType(i) == vtkSMStringVectorProperty::STRING)
        {
        svp->SetUncheckedElement(i, Value.toString().toAscii().data());
        }
      }
    Property->UpdateDependentDomains();
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
  if(!Property)
    {
    return enumerations;
    }

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

  int num = Value.size();

  if(dvp)
    {
    double *dvalues = new double[num+1];
    for(int i=0; i<num; i++)
      {
      bool ok = true;
      double v = Value[i].toDouble(&ok);
      dvalues[i] = ok? v : 0.0;
      }
    dvp->SetNumberOfElements(num);
    if (num > 0)
      {
      dvp->SetElements(dvalues);
      }
    delete[] dvalues;
    }
  else if(ivp)
    {
    int *ivalues = new int[num+1];
    for(int i=0; i<num; i++)
      {
      bool ok = true;
      int v = Value[i].toInt(&ok);
      ivalues[i] = ok? v : 0;
      }
    ivp->SetNumberOfElements(num);
    if (num>0)
      {
      ivp->SetElements(ivalues);
      }
    delete []ivalues;
    }
  else if(svp)
    {
    const char** cvalues = new const char*[num];
    vtkstd::string *str_values= new vtkstd::string[num];
    for (int cc=0; cc < num; cc++)
      {
      str_values[cc] = Value[cc].toString().toAscii().data();
      cvalues[cc] = str_values[cc].c_str();
      }

    svp->SetElements(num, cvalues);
    delete []cvalues;
    delete []str_values;
    }
  else if(idvp)
    {
    vtkIdType* idvalues = new vtkIdType[num+1];
    for(int i=0; i<num; i++)
      {
      bool ok = true;
      vtkIdType v;
#if defined(VTK_USE_64BIT_IDS)
      v = Value[i].toLongLong(&ok);
#else
      v = Value[i].toInt(&ok);
#endif
      idvalues[i] = ok? v : 0;
      }
    idvp->SetNumberOfElements(num);
    if (num>0)
      {
      idvp->SetElements(idvalues);
      }
    delete[] idvalues;
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
  
  int num = Value.size();

  if(dvp)
    {
    for(int i=0; i<num; i++)
      {
      bool ok = true;
      double v = Value[i].toDouble(&ok);
      if(ok)
        {
        dvp->SetUncheckedElement(i, v);
        }
      }
    }
  else if(ivp)
    {
    for(int i=0; i<num; i++)
      {
      bool ok = true;
      int v = Value[i].toInt(&ok);
      if(ok)
        {
        ivp->SetUncheckedElement(i, v);
        }
      }
    }
  else if(svp)
    {
    for(int i=0; i<num; i++)
      {
      QString v = Value[i].toString();
      if(!v.isNull())
        {
        svp->SetUncheckedElement(i, v.toAscii().data());
        }
      }
    }
  else if(idvp)
    {
    for(int i=0; i<num; i++)
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
  if(!Property)
    {
    return domains;
    }
  
  vtkSMDoubleRangeDomain* DoubleDomain = NULL;
  vtkSMIntRangeDomain* IntDomain = NULL;
  
  vtkSMDomainIterator* iter = Property->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd() && !DoubleDomain && !IntDomain)
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
    vtkSMArrayRangeDomain* arrayDomain;
    arrayDomain = vtkSMArrayRangeDomain::SafeDownCast(DoubleDomain);

    unsigned int numElems = dvp->GetNumberOfElements();
    for(unsigned int i=0; i<numElems; i++)
      {
      QList<QVariant> domain;
      int exists1, exists2;
      int which = i;
      if(arrayDomain)
        {
        which = 0;
        }
      double min = DoubleDomain->GetMinimum(which, exists1);
      double max = DoubleDomain->GetMaximum(which, exists2);
      domain.push_back(exists1 ? min : QVariant());
      domain.push_back(exists2 ? max : QVariant());
      domains.push_back(domain);
      }
    }
  else if(IntDomain)
    {
    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(Property);

    unsigned int numElems = ivp->GetNumberOfElements();
    vtkSMExtentDomain* extDomain = vtkSMExtentDomain::SafeDownCast(IntDomain);
    
    for(unsigned int i=0; i<numElems; i++)
      {
      int which = i;
      if(extDomain)
        {
        which /= 2;
        }
      else
        {  
        // one min/max for all elements
        which = 0;
        }
      QList<QVariant> domain;
      int exists1, exists2;
      int min = IntDomain->GetMinimum(which, exists1);
      int max = IntDomain->GetMaximum(which, exists2);
      domain.push_back(exists1 ? min : QVariant());
      domain.push_back(exists2 ? max : QVariant());
      domains.push_back(domain);
      }
    }

  return domains;
}

QVariant pqSMAdaptor::getMultipleElementProperty(vtkSMProperty* Property,
                                                 unsigned int Index)
{
  QVariant var;
  
  vtkSMDoubleVectorProperty* dvp = NULL;
  vtkSMIntVectorProperty* ivp = NULL;
  vtkSMIdTypeVectorProperty* idvp = NULL;
  vtkSMStringVectorProperty* svp = NULL;

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

  if(dvp)
    {
    bool ok = true;
    double v = Value.toDouble(&ok);
    if(ok)
      {
      dvp->SetElement(Index, v);
      }
    }
  else if(ivp)
    {
    bool ok = true;
    int v = Value.toInt(&ok);
    if (!ok && Value.canConvert(QVariant::Bool))
      {
      v = Value.toBool()? 1 : 0;
      ok = true;
      }
    if(ok)
      {
      ivp->SetElement(Index, v);
      }
    }
  else if(svp)
    {
    QString v = Value.toString();
    if(!v.isNull())
      {
      svp->SetElement(Index, v.toAscii().data());
      }
    }
  else if(idvp)
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
  if(!Property)
    {
    return domain;
    }
  
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

  int which = 0;
  vtkSMExtentDomain* extDomain = vtkSMExtentDomain::SafeDownCast(IntDomain);
  if(extDomain)
    {
    which = Index/2;
    }

  if(DoubleDomain)
    {
    int exists1, exists2;
    double min = DoubleDomain->GetMinimum(which, exists1);
    double max = DoubleDomain->GetMaximum(which, exists2);
    domain.push_back(exists1 ? min : QVariant());
    domain.push_back(exists2 ? max : QVariant());
    }
  else if(IntDomain)
    {
    int exists1, exists2;
    int min = IntDomain->GetMinimum(which, exists1);
    int max = IntDomain->GetMaximum(which, exists2);
    domain.push_back(exists1 ? min : QVariant());
    domain.push_back(exists2 ? max : QVariant());
    }

  return domain;
}

QStringList pqSMAdaptor::getFileListProperty(vtkSMProperty* Property)
{
  QStringList files;

  vtkSMStringVectorProperty* svp;
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if (svp)
    {
    for (unsigned int i = 0; i < svp->GetNumberOfElements(); i++)
      {
      files.append(svp->GetElement(i));
      }
    }

  return files;
}

void pqSMAdaptor::setFileListProperty(vtkSMProperty* Property,
                                      QStringList Value)
{
  vtkSMStringVectorProperty* svp;
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if (!svp) return;

  unsigned int i = 0;
  foreach (QString file, Value)
    {
    if (!svp->GetRepeatCommand() && (i >= svp->GetNumberOfElements())) break;
    svp->SetElement(i, file.toAscii().data());
    i++;
    }

  if (static_cast<int>(svp->GetNumberOfElements()) != Value.size())
    {
    svp->SetNumberOfElements(svp->GetNumberOfElements());
    }
}

void pqSMAdaptor::setUncheckedFileListProperty(vtkSMProperty* Property,
                                               QStringList Value)
{
  vtkSMStringVectorProperty* svp;
  svp = vtkSMStringVectorProperty::SafeDownCast(Property);

  if (!svp) return;

  unsigned int i = 0;
  foreach (QString file, Value)
    {
    if (!svp->GetRepeatCommand() && (i >= svp->GetNumberOfElements())) break;
    svp->SetUncheckedElement(i, file.toAscii().data());
    i++;
    }

  if (static_cast<int>(svp->GetNumberOfElements()) != Value.size())
    {
    svp->SetNumberOfElements(svp->GetNumberOfElements());
    }
  Property->UpdateDependentDomains();
}

QString pqSMAdaptor::getFieldSelectionMode(vtkSMProperty* prop)
{
  QString ret;
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMEnumerationDomain* domain =
    vtkSMEnumerationDomain::SafeDownCast(prop->GetDomain("field_list"));
  
  if(Property && domain)
    {
    int which = QString(Property->GetElement(3)).toInt();
    int numEntries = domain->GetNumberOfEntries();
    for(int i=0; i<numEntries; i++)
      {
      if(domain->GetEntryValue(i) == which)
        {
        ret = domain->GetEntryText(i);
        break;
        }
      }
    }
  return ret;
}

void pqSMAdaptor::setFieldSelectionMode(vtkSMProperty* prop, 
                                             const QString& val)
{
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMEnumerationDomain* domain =
    vtkSMEnumerationDomain::SafeDownCast(prop->GetDomain("field_list"));
  
  if(Property && domain)
    {
    int numEntries = domain->GetNumberOfEntries();
    for(int i=0; i<numEntries; i++)
      {
      if(val == domain->GetEntryText(i))
        {
        Property->SetElement(3, 
           QString("%1").arg(domain->GetEntryValue(i)).toAscii().data());
        break;
        }
      }
    }
}

void pqSMAdaptor::setUncheckedFieldSelectionMode(vtkSMProperty* prop, 
                                             const QString& val)
{
  if(!prop)
    {
    return;
    }
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMEnumerationDomain* domain =
    vtkSMEnumerationDomain::SafeDownCast(prop->GetDomain("field_list"));
  
  if(Property && domain)
    {
    int numEntries = domain->GetNumberOfEntries();
    for(int i=0; i<numEntries; i++)
      {
      if(val == domain->GetEntryText(i))
        {
        Property->SetUncheckedElement(3, 
           QString("%1").arg(domain->GetEntryValue(i)).toAscii().data());
        break;
        }
      }
    Property->UpdateDependentDomains();
    }
}

QList<QString> pqSMAdaptor::getFieldSelectionModeDomain(vtkSMProperty* prop)
{
  QList<QString> types;
  if(!prop)
    {
    return types;
    }

  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMEnumerationDomain* domain =
    vtkSMEnumerationDomain::SafeDownCast(prop->GetDomain("field_list"));
  
  if(Property && domain)
    {
    int numEntries = domain->GetNumberOfEntries();
    for(int i=0; i<numEntries; i++)
      {
      types.append(domain->GetEntryText(i));
      }
    }
  return types;
}


QString pqSMAdaptor::getFieldSelectionScalar(vtkSMProperty* prop)
{
  QString ret;
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  
  if(Property)
    {
    ret = Property->GetElement(4);
    }
  return ret;
}

void pqSMAdaptor::setFieldSelectionScalar(vtkSMProperty* prop, 
                                              const QString& val)
{
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  
  if(Property)
    {
    Property->SetElement(4, val.toAscii().data());
    }
}

void pqSMAdaptor::setUncheckedFieldSelectionScalar(vtkSMProperty* prop, 
                                              const QString& val)
{
  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  
  if(Property)
    {
    Property->SetUncheckedElement(4, val.toAscii().data());
    Property->UpdateDependentDomains();
    }
}

//-----------------------------------------------------------------------------
QList<QString> pqSMAdaptor::getFieldSelectionScalarDomain(vtkSMProperty* prop)
{
  QList<QString> types;
  if(!prop)
    {
    return types;
    }

  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMArrayListDomain* domain = prop ?
    vtkSMArrayListDomain::SafeDownCast(prop->GetDomain("array_list")) : 0;
  
  if(Property && domain)
    {
    int numEntries = domain->GetNumberOfStrings();
    for(int i=0; i<numEntries; i++)
      {
      types.append(domain->GetString(i));
      }
    }
  return types;
}

//-----------------------------------------------------------------------------
QList<QPair<QString, bool> > pqSMAdaptor::getFieldSelectionScalarDomainWithPartialArrays(
  vtkSMProperty* prop)
{
  QList<QPair<QString, bool> > types;
  if (!prop)
    {
    return types;
    }

  vtkSMStringVectorProperty* Property =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  vtkSMArrayListDomain* domain = prop ?
    vtkSMArrayListDomain::SafeDownCast(prop->GetDomain("array_list")) : 0;
  
  if(Property && domain)
    {
    int numEntries = domain->GetNumberOfStrings();
    for(int i=0; i<numEntries; i++)
      {
      types.append(QPair<QString, bool>(domain->GetString(i), domain->IsArrayPartial(i)!=0));
      }
    }
  return types;
}

//-----------------------------------------------------------------------------
QList<QString> pqSMAdaptor::getDomainTypes(vtkSMProperty* property)
{
  QList<QString> types;
  if(!property)
    {
    return types;
    }
  
  vtkSMDomainIterator* iter = property->NewDomainIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMDomain* d = iter->GetDomain();
    QString classname = d->GetClassName();
    if (!types.contains(classname))
      {
      types.push_back(classname);
      }
    }
  iter->Delete();
  return types;
}
