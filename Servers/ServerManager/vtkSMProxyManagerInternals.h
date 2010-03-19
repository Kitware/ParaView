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
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMLink.h"
#include "vtkSMProxy.h"
#include "vtkSMProxySelectionModel.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtksys/ios/sstream>
#include "vtkStdString.h"

class vtkSMProxyManagerElement : public vtkSmartPointer<vtkPVXMLElement>
{
  typedef vtkSmartPointer<vtkPVXMLElement> Superclass;
public:
  bool Custom; // custom is true for definitions not added by vtkSMXMLParser.
  vtkSMProxyManagerElement() :Custom(false) {}
  vtkSMProxyManagerElement(vtkPVXMLElement* elem)
    {
    this->Custom = false;
    this->Superclass::operator=(elem);
    }
  vtkSMProxyManagerElement(vtkPVXMLElement* elem, bool custom)
    {
    this->Custom = custom;
    this->Superclass::operator=(elem);
    }

  // Description:
  // Assign object to reference.  This removes any reference to an old
  // object.
  vtkSMProxyManagerElement& operator=(vtkPVXMLElement* r)
    {
    this->Custom = false;
    this->Superclass::operator=(r);
    return *this;
    }

  // Description:
  // Returns true if the defintions match.
  bool DefinitionsMatch(vtkPVXMLElement* other)
    {
    vtkPVXMLElement* self = this->GetPointer();
    if (self == other)
      {
      return true;
      }
    if (!other || !self)
      {
      return false;
      }
    vtksys_ios::ostringstream selfstream;
    vtksys_ios::ostringstream otherstream;
    self->PrintXML(selfstream, vtkIndent());
    other->PrintXML(otherstream, vtkIndent());
    return (selfstream.str() == otherstream.str());
    }

};

// Sub-classed to avoid symbol length explosion.
class vtkSMProxyManagerElementMapType:
  public vtkstd::map<vtkStdString, vtkSMProxyManagerElement> {};

class vtkSMProxyManagerProxyInfo : public vtkObjectBase
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;
  unsigned long ModifiedObserverTag;
  unsigned long StateChangedObserverTag;
  unsigned long UpdateObserverTag;
  unsigned long UpdateInformationObserverTag;

  static vtkSMProxyManagerProxyInfo* New()
    {
    return new vtkSMProxyManagerProxyInfo();
    }

  // Description:
  // this needs to be overridden otherwise vtkDebugLeaks warnings are objects
  // are destroyed.
  void UnRegister()
    {
    int refcount = this->GetReferenceCount()-1;
    this->SetReferenceCount(refcount);
    if (refcount <= 0)
      {
#ifdef VTK_DEBUG_LEAKS
      vtkDebugLeaks::DestructClass("vtkSMProxyManagerProxyInfo");
#endif
      delete this;
      }
    }
  virtual void UnRegister(vtkObjectBase *)
    { this->UnRegister(); }

private:
  vtkSMProxyManagerProxyInfo()
    {
    this->ModifiedObserverTag = 0;
    this->StateChangedObserverTag = 0;
    this->UpdateObserverTag = 0;
    this->UpdateInformationObserverTag = 0;
    }
  ~vtkSMProxyManagerProxyInfo()
    {
    // Remove observers.
    if (this->ModifiedObserverTag && this->Proxy.GetPointer())
      {
      this->Proxy.GetPointer()->RemoveObserver(this->ModifiedObserverTag);
      this->ModifiedObserverTag = 0;
      }
    if (this->StateChangedObserverTag && this->Proxy.GetPointer())
      {
      this->Proxy.GetPointer()->RemoveObserver(this->StateChangedObserverTag);
      this->StateChangedObserverTag = 0;
      }
    if (this->UpdateObserverTag && this->Proxy.GetPointer())
      {
      this->Proxy.GetPointer()->RemoveObserver(this->UpdateObserverTag);
      this->UpdateObserverTag = 0;
      }
    if (this->UpdateInformationObserverTag && this->Proxy.GetPointer())
      {
      this->Proxy.GetPointer()->RemoveObserver(
        this->UpdateInformationObserverTag);
      this->UpdateInformationObserverTag = 0;
      }
    }
};

//-----------------------------------------------------------------------------
class vtkSMProxyManagerProxyListType :
  public vtkstd::vector<vtkSmartPointer<vtkSMProxyManagerProxyInfo> >
{
public:
  // Returns if the proxy exists in  this vector.
  bool Contains(vtkSMProxy* proxy) 
    {
    vtkSMProxyManagerProxyListType::iterator iter =
      this->begin();
    for (; iter != this->end(); ++iter)
      {
      if (iter->GetPointer()->Proxy == proxy)
        {
        return true;
        }
      }
    return false;
    }
  vtkSMProxyManagerProxyListType::iterator Find(vtkSMProxy* proxy)
    {
    vtkSMProxyManagerProxyListType::iterator iter =
      this->begin();
    for (; iter != this->end(); ++iter)
      {
      if (iter->GetPointer()->Proxy.GetPointer() == proxy)
        {
        return iter;
        }
      }
    return this->end();
    }
};

//-----------------------------------------------------------------------------
class vtkSMProxyManagerProxyMapType:
  public vtkstd::map<vtkStdString, vtkSMProxyManagerProxyListType> {};

//-----------------------------------------------------------------------------
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

  // Data structure to save registered links.
  typedef vtkstd::map<vtkStdString, vtkSmartPointer<vtkSMLink> >
    LinkType;
  LinkType RegisteredLinkMap;

  // Data structure for selection models.
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkSMProxySelectionModel> >
    SelectionModelsType;
  SelectionModelsType SelectionModels;

  // Data structure for storing GlobalPropertiesManagers.
  typedef vtkstd::map<vtkstd::string,
          vtkSmartPointer<vtkSMGlobalPropertiesManager> >
            GlobalPropertiesManagersType;
  GlobalPropertiesManagersType GlobalPropertiesManagers;

  // Helper method to retrieve the proxy element.
  vtkPVXMLElement* GetProxyElement(const char* groupName, const char* proxyName)
    {
    if (!groupName || !proxyName)
      {
      return 0;
      }
    vtkPVXMLElement* element = 0;

    // Find the XML element from the proxy.
    // 
    vtkSMProxyManagerInternals::GroupMapType::iterator it =
      this->GroupMap.find(groupName);
    if (it != this->GroupMap.end())
      {
      vtkSMProxyManagerElementMapType::iterator it2 =
        it->second.find(proxyName);

      if (it2 != it->second.end())
        {
        element = it2->second.GetPointer();
        }
      }

    return element;
    }
};

#endif

