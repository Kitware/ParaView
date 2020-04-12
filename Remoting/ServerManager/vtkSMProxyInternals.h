/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSMProxyInternals_h
#define vtkSMProxyInternals_h

#include "vtkClientServerStream.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLink.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <map>
#include <string>
#include <vector>

//---------------------------------------------------------------------------
// Internal data structure for storing object IDs, server IDs and
// properties. Each property has associated attributes:
// * ModifiedFlag : has the property been modified since last update (push)
// * DoUpdate : should the property be updated (pushed) during UpdateVTKObjects
// * ObserverTag : the tag returned by AddObserver(). Used to remove the
// observer.
struct vtkSMProxyInternals
{
  struct PropertyInfo
  {
    PropertyInfo()
    {
      this->ModifiedFlag = 0;
      this->ObserverTag = 0;
    };
    vtkSmartPointer<vtkSMProperty> Property;
    int ModifiedFlag;
    unsigned int ObserverTag;
  };
  // Note that the name of the property is the map key. That is the
  // only place where name is stored
  typedef std::map<std::string, PropertyInfo> PropertyInfoMap;
  PropertyInfoMap Properties;

  // This vector keeps track of the order in which properties
  // were added for the Property iterator
  std::vector<std::string> PropertyNamesInOrder;

  std::vector<vtkSmartPointer<vtkSMPropertyGroup> > PropertyGroups;

  std::vector<int> ServerIDs;

  typedef std::map<std::string, vtkSmartPointer<vtkSMProxy> > ProxyMap;
  ProxyMap SubProxies;

  struct ConnectionInfo
  {
    ConnectionInfo(vtkSMProperty* prop, vtkSMProxy* prox)
      : Property(prop)
      , Proxy(prox){};
    vtkWeakPointer<vtkSMProperty> Property;
    vtkWeakPointer<vtkSMProxy> Proxy;
  };
  std::vector<ConnectionInfo> Consumers;
  std::vector<ConnectionInfo> Producers;

  struct ExposedPropertyInfo
  {
    std::string SubProxyName;
    std::string PropertyName;
  };

  // Map for exposed properties. The key is the exposed property name,
  // value is a ExposedPropertyInfo object which indicates the subproxy name
  // and the property name in that subproxy.
  typedef std::map<std::string, ExposedPropertyInfo> ExposedPropertyInfoMap;
  ExposedPropertyInfoMap ExposedProperties;

  // Vector of vtkSMProxyLink for shared properties among subproxies.
  typedef std::vector<vtkSmartPointer<vtkSMProxyLink> > SubProxyLinksType;
  SubProxyLinksType SubProxyLinks;

  // Annotation map
  typedef std::map<std::string, std::string> AnnotationMap;
  AnnotationMap Annotations;
  bool EnableAnnotationPush;

  // Setup default values
  vtkSMProxyInternals() { this->EnableAnnotationPush = true; }
};

#endif

// VTK-HeaderTest-Exclude: vtkSMProxyInternals.h
