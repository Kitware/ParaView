/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManagerInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSMProxyManagerInternals_h
#define __vtkSMProxyManagerInternals_h

#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMLink.h"
#include "vtkSMProxy.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include "vtkStdString.h"

// Sub-classed to avoid symbol length explosion.
class vtkSMProxyManagerElementMapType:
  public vtkstd::map<vtkStdString, vtkSmartPointer<vtkPVXMLElement> > {};

struct ProxyInfo
{
  ProxyInfo()
    {
    this->ModifiedObserverTag = 0;
    this->UpdateObserverTag = 0;
    }
  ~ProxyInfo()
    {
    // Remove observers.
    if (this->ModifiedObserverTag && this->Proxy.GetPointer())
      {
      this->Proxy.GetPointer()->RemoveObserver(this->ModifiedObserverTag);
      this->ModifiedObserverTag = 0;
      }
    if (this->UpdateObserverTag && this->Proxy.GetPointer())
      {
      this->Proxy.GetPointer()->RemoveObserver(this->UpdateObserverTag);
      this->UpdateObserverTag = 0;
      }
    }

  vtkSmartPointer<vtkSMProxy> Proxy;
  unsigned long ModifiedObserverTag;
  unsigned long UpdateObserverTag;
};
class vtkSMProxyManagerProxyMapType:
  public vtkstd::map<vtkStdString, ProxyInfo > {};


struct vtkSMProxyManagerInternals
{
  // This data structure stores the XML elements (prototypes) from which
  // proxies and properties are instantiated and initialized.
  typedef 
  vtkstd::map<vtkStdString, vtkSMProxyManagerElementMapType> GroupMapType;
  GroupMapType GroupMap;

  // This data structure stores actual proxy instances grouped in
  // collections.
  typedef 
  vtkstd::map<vtkStdString, vtkSMProxyManagerProxyMapType> ProxyGroupType;
  ProxyGroupType RegisteredProxyMap;

  // This data structure stores a set of proxies that have been modified.
  typedef vtkstd::set<vtkSMProxy*> SetOfProxies;
  SetOfProxies ModifiedProxies;

  typedef vtkstd::map<vtkStdString, vtkSmartPointer<vtkPVXMLElement> >
     DefinitionType;
  DefinitionType CompoundProxyDefinitions;

  // Data structure to save registered links.
  typedef vtkstd::map<vtkStdString, vtkSmartPointer<vtkSMLink> >
    LinkType;
  LinkType RegisteredLinkMap;
};

#endif

