// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
  std::map<vtkTypeUInt32, vtkSmartPointer<vtkPVXMLElement>> XMLCacheMap;
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
  std::map<vtkTypeUInt32, vtkSmartPointer<vtkPVXMLElement>>::iterator iter;
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
