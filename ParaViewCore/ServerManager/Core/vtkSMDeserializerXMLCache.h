/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDeserializerXMLCache.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDeserializerXMLCache - deserializes proxies from their XML states.
// .SECTION Description
// vtkSMDeserializerXMLCache is used to deserialize proxies from previously
// stored XML states.

#ifndef __vtkSMDeserializerXMLCache_h
#define __vtkSMDeserializerXMLCache_h

#include "vtkSMDeserializerXML.h"

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMSession;

class VTK_EXPORT vtkSMDeserializerXMLCache : public vtkSMDeserializerXML
{
public:
  static vtkSMDeserializerXMLCache* New();
  vtkTypeMacro(vtkSMDeserializerXMLCache, vtkSMDeserializerXML);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allow to register a given XML state for a given proxy GlobalId
  virtual void CacheXMLProxyState(vtkTypeUInt32 id, vtkPVXMLElement* xml);

//BTX
protected:
  vtkSMDeserializerXMLCache();
  ~vtkSMDeserializerXMLCache();

  // Friend to access NewProxy().
  friend class vtkSMProxyLocator;

  // Description:
  // Locate the XML for the proxy with the given id.
  virtual vtkPVXMLElement* LocateProxyElement(vtkTypeUInt32 id);

private:
  vtkSMDeserializerXMLCache(const vtkSMDeserializerXMLCache&); // Not implemented
  void operator=(const vtkSMDeserializerXMLCache&); // Not implemented

  class vtkInternal;
  vtkInternal* Internals;
//ETX
};

#endif
