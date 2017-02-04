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
/**
 * @class   vtkSMDeserializerXMLCache
 * @brief   deserializes proxies from their XML states.
 *
 * vtkSMDeserializerXMLCache is used to deserialize proxies from previously
 * stored XML states.
*/

#ifndef vtkSMDeserializerXMLCache_h
#define vtkSMDeserializerXMLCache_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDeserializerXML.h"

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMSession;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMDeserializerXMLCache : public vtkSMDeserializerXML
{
public:
  static vtkSMDeserializerXMLCache* New();
  vtkTypeMacro(vtkSMDeserializerXMLCache, vtkSMDeserializerXML);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Allow to register a given XML state for a given proxy GlobalId
   */
  virtual void CacheXMLProxyState(vtkTypeUInt32 id, vtkPVXMLElement* xml);

protected:
  vtkSMDeserializerXMLCache();
  ~vtkSMDeserializerXMLCache();

  // Friend to access NewProxy().
  friend class vtkSMProxyLocator;

  /**
   * Locate the XML for the proxy with the given id.
   */
  virtual vtkPVXMLElement* LocateProxyElement(vtkTypeUInt32 id) VTK_OVERRIDE;

private:
  vtkSMDeserializerXMLCache(const vtkSMDeserializerXMLCache&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMDeserializerXMLCache&) VTK_DELETE_FUNCTION;

  class vtkInternal;
  vtkInternal* Internals;
};

#endif
