/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyLink.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropertyLink.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkStdString.h"

#include <vtkstd/list>

vtkStandardNewMacro(vtkSMPropertyLink);
vtkCxxRevisionMacro(vtkSMPropertyLink, "1.1");
//-----------------------------------------------------------------------------
struct vtkSMPropertyLinkInternals
{
  struct LinkedProperty
    {
    LinkedProperty(vtkSMProxy* proxy, const char* pname, int updateDir) :
      Proxy(proxy), PropertyName(pname), UpdateDirection(updateDir)
    {
    }
    ~LinkedProperty()
      {
      if (this->Observer && this->Proxy.GetPointer())
        {
        this->Proxy.GetPointer()->RemoveObserver(Observer);
        this->Observer = 0;
        }
      }

    vtkSmartPointer<vtkSMProxy> Proxy;
    vtkStdString PropertyName;
    int UpdateDirection;
    vtkCommand* Observer;
    };
  typedef vtkstd::list<LinkedProperty> LinkedPropertyType;
  LinkedPropertyType LinkedProperties;
};

//-----------------------------------------------------------------------------
vtkSMPropertyLink::vtkSMPropertyLink()
{
  this->Internals = new vtkSMPropertyLinkInternals;
}

//-----------------------------------------------------------------------------
vtkSMPropertyLink::~vtkSMPropertyLink()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::AddLinkedProperty(vtkSMProxy* proxy, const char* pname,
  int updateDir)
{
  int addToList = 1;
  int addObserver = updateDir & INPUT;
  
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for (; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->Proxy.GetPointer() == proxy && 
      iter->PropertyName == pname)
      {
      if (iter->UpdateDirection != updateDir)
        {
        iter->UpdateDirection = updateDir;
        }
      else
        {
        addObserver = 0;
        }
      addToList = 0;
      }
    }
  if (addToList)
    {
    vtkSMPropertyLinkInternals::LinkedProperty link(proxy, pname, updateDir);
    link.Observer = this->Observer;
    this->Internals->LinkedProperties.push_back(link);
    }

  if (addObserver)
    {
    this->ObserveProxyUpdates(proxy);
    }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::UpdateProperties(vtkSMProxy* fromProxy, const char* pname)
{
  if (!fromProxy)
    {
    return;
    }

  vtkSMProperty* fromProp = fromProxy->GetProperty(pname);
  if (!fromProp)
    {
    return;
    }

  // First verify that the property that triggerred this call is indeed
  // an input property.
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter;
  int propagate = 0;
  for (iter = this->Internals->LinkedProperties.begin(); 
    iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->UpdateDirection & INPUT && iter->Proxy.GetPointer() == fromProxy
      && iter->PropertyName == pname)
      {
      propagate = 1;
      break;
      }
    }

  if (!propagate)
    {
    return;
    }
 
  // Propagate the changes.
  for (iter = this->Internals->LinkedProperties.begin(); 
    iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if (iter->UpdateDirection & OUTPUT)
      {
      vtkSMProperty* toProp = iter->Proxy.GetPointer()->GetProperty(
        iter->PropertyName);
      if (toProp)
        {
        toProp->Copy(fromProp);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::UpdateVTKObjects(vtkSMProxy* caller)
{
  vtkSMPropertyLinkInternals::LinkedPropertyType::iterator iter =
    this->Internals->LinkedProperties.begin();
  for(; iter != this->Internals->LinkedProperties.end(); ++iter)
    {
    if ((iter->Proxy.GetPointer() != caller) && 
        (iter->UpdateDirection & OUTPUT))
      {
      iter->Proxy.GetPointer()->UpdateVTKObjects();
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMPropertyLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
