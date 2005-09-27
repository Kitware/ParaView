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
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include "vtkStdString.h"

// Sub-classed to avoid symbol length explosion.
class vtkSMProxyManagerElementMapType:
  public vtkstd::map<vtkStdString, vtkSmartPointer<vtkPVXMLElement> > {};

class vtkSMProxyManagerProxyMapType:
  public vtkstd::map<vtkStdString, vtkSmartPointer<vtkSMProxy> > {};

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
};

#endif

