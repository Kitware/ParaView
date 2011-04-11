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

#ifndef __vtkSMProxyInternals_h
#define __vtkSMProxyInternals_h

#include "vtkClientServerStream.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyLink.h"
#include "vtkWeakPointer.h"

#include <vtkstd/map>
#include <vtkstd/vector>
#include "vtkStdString.h"

//---------------------------------------------------------------------------
// Internal data structure for storing object IDs, server IDs and
// properties. Each property has associated attributes: 
// * ModifiedFlag : has the property been modified since last update (push)
// * DoUpdate : should the propery be updated (pushed) during UpdateVTKObjects 
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
  typedef vtkstd::map<vtkStdString,  PropertyInfo> PropertyInfoMap;
  PropertyInfoMap Properties;

  // This vector keeps track of the order in which properties
  // were added for the Property iterator
  vtkstd::vector<vtkStdString> PropertyNamesInOrder;

  vtkstd::vector<int> ServerIDs;

  typedef vtkstd::map<vtkStdString,  vtkSmartPointer<vtkSMProxy> > ProxyMap;
  ProxyMap SubProxies;

  struct ConnectionInfo
  {
    ConnectionInfo(vtkSMProperty* prop, vtkSMProxy* prox) : Property(prop),
      Proxy(prox) {};
    vtkWeakPointer<vtkSMProperty> Property;
    vtkWeakPointer<vtkSMProxy> Proxy;
  };
  vtkstd::vector<ConnectionInfo> Consumers;
  vtkstd::vector<ConnectionInfo> Producers;

  struct ExposedPropertyInfo
    {
    vtkStdString SubProxyName;
    vtkStdString PropertyName;
    };

  // Map for exposed properties. The key is the exposed property name,
  // value is a ExposedPropertyInfo object which indicates the subproxy name
  // and the property name in that subproxy.
  typedef vtkstd::map<vtkStdString, ExposedPropertyInfo> ExposedPropertyInfoMap;
  ExposedPropertyInfoMap ExposedProperties;

  // Vector of vtkSMProxyLink for shared properties among subproxies.
  typedef  vtkstd::vector<vtkSmartPointer<vtkSMProxyLink> > SubProxyLinksType;
  SubProxyLinksType SubProxyLinks;
};

#endif

