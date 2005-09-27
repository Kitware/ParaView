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

#include <vtkstd/map>
#include <vtkstd/vector>
#include "vtkStdString.h"
#include <vtkstd/set>

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
  vtkstd::vector<vtkClientServerID > IDs;
  vtkstd::vector<int> ServerIDs;
  // Note that the name of the property is the map key. That is the
  // only place where name is stored
  typedef vtkstd::map<vtkStdString,  PropertyInfo> PropertyInfoMap;
  PropertyInfoMap Properties;

  typedef vtkstd::map<vtkStdString,  vtkSmartPointer<vtkSMProxy> > ProxyMap;
  ProxyMap SubProxies;

  struct ConsumerInfo
  {
    ConsumerInfo(vtkSMProperty* prop, vtkSMProxy* prox) : Property(prop),
      Proxy(prox) {};
    vtkSMProperty* Property;
    vtkSMProxy* Proxy;
  };
  vtkstd::vector<ConsumerInfo> Consumers;
  
  // This is a set of exposed property names. 
  vtkstd::set<vtkStdString> ExposedPropertyNames;
};

#endif

