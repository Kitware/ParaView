/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDeserializerXMLCache.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDeserializerXMLCache.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSmartPointer.h"

#include <map>
#include <vtkSmartPointer.h>

//*****************************************************************************
//                     Internal class definition
//*****************************************************************************
class vtkSMDeserializerXMLCache::vtkInternal
{
public:
  std::map<vtkTypeUInt32, vtkSmartPointer<vtkPVXMLElement> > XMLCacheMap;
};
//*****************************************************************************
vtkStandardNewMacro(vtkSMDeserializerXMLCache);
//----------------------------------------------------------------------------
vtkSMDeserializerXMLCache::vtkSMDeserializerXMLCache()
{
  this->Internals = new vtkInternal;
}

//----------------------------------------------------------------------------
vtkSMDeserializerXMLCache::~vtkSMDeserializerXMLCache()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMDeserializerXMLCache::LocateProxyElement(vtkTypeUInt32 id)
{
  return this->Internals->XMLCacheMap[id].GetPointer();
}

//----------------------------------------------------------------------------
void vtkSMDeserializerXMLCache::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  std::map<vtkTypeUInt32, vtkSmartPointer<vtkPVXMLElement> >::iterator iter;
  for (iter = this->Internals->XMLCacheMap.begin(); iter != this->Internals->XMLCacheMap.end();
       iter++)
  {
    os << indent << "Proxy " << iter->first << " state:" << endl;
    iter->second.GetPointer()->PrintXML(os, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
void vtkSMDeserializerXMLCache::CacheXMLProxyState(vtkTypeUInt32 id, vtkPVXMLElement* xml)
{
  this->Internals->XMLCacheMap[id] = xml;
}
