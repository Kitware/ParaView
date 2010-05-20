/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyLocator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyLocator.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMDeserializer.h"
#include "vtkSMProxy.h"

#include <vtkstd/map>

class vtkSMProxyLocator::vtkInternal
{
public:
  typedef vtkstd::map<int, vtkSmartPointer<vtkSMProxy> > ProxiesType;
  ProxiesType Proxies;
};

vtkStandardNewMacro(vtkSMProxyLocator);
vtkCxxSetObjectMacro(vtkSMProxyLocator, Deserializer, vtkSMDeserializer);
//----------------------------------------------------------------------------
vtkSMProxyLocator::vtkSMProxyLocator()
{
  this->ConnectionID = 0;
  this->Internal = new vtkInternal();
  this->Deserializer = 0;
  this->ReviveProxies = 0;
}

//----------------------------------------------------------------------------
vtkSMProxyLocator::~vtkSMProxyLocator()
{
  delete this->Internal;
  this->SetDeserializer(0);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyLocator::LocateProxy(int id)
{
  vtkInternal::ProxiesType::iterator iter = this->Internal->Proxies.find(id);
  if (iter != this->Internal->Proxies.end())
    {
    return iter->second.GetPointer();
    }
 
  vtkSMProxy* newProxy = this->NewProxy(id);
  if (newProxy)
    {
    this->Internal->Proxies[id].TakeReference(newProxy);
    }
  return newProxy;
}

//----------------------------------------------------------------------------
void vtkSMProxyLocator::Clear()
{
  this->Internal->Proxies.clear();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyLocator::NewProxy(int id)
{
  if (this->Deserializer)
    {
    // Ask the deserializer to create a new proxy with the given id. The
    // deserializer will locate the XML for the proxy with that id, and load the
    // state on it and then return this fresh proxy, if possible.
    return this->Deserializer->NewProxy(id, this);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMProxyLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ReviveProxies: " << this->ReviveProxies << endl;
  os << indent << "Deserializer: " << this->Deserializer << endl;
}


