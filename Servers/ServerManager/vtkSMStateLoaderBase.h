/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateLoaderBase.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStateLoaderBase - base class for all proxy manager state loaders.
// .SECTION Description
// vtkSMStateLoaderBase is a base class for all proxy manager state loaders.
// State loaders are used to load proxy manager state xmls.

#ifndef __vtkSMStateLoaderBase_h
#define __vtkSMStateLoaderBase_h

#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProxy;

class VTK_EXPORT vtkSMStateLoaderBase : public vtkSMObject
{
public:
  vtkTypeRevisionMacro(vtkSMStateLoaderBase, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Either create a new proxy or returns one from the map
  // of existing properties. Newly created proxies are stored
  // in the map with the id as the key.
  // Refer to the documentation of UseExistingProxies.
  virtual vtkSMProxy* NewProxy(int id);
  virtual vtkSMProxy* NewProxyFromElement(vtkPVXMLElement* proxyElement, int id);

  // Description:
  // Get/Set the connection ID for the connection on which the state is
  // loaded. By default it is set to NullConnectionID.
  vtkSetMacro(ConnectionID, vtkIdType);
  vtkGetMacro(ConnectionID, vtkIdType);

  // Description:
  // Get/Set if the proxies are to be revived, if the state has sufficient
  // information needed to revive proxies (such as server-side object IDs etc).
  // By default, this is set to 0.
  vtkSetMacro(ReviveProxies, int);
  vtkGetMacro(ReviveProxies, int);

  // Description:
  // Clears all internal references to created proxies from the map.
  // This is the map to which proxies are automatically added
  // as a result of calling NewProxy() or NewProxyFromElement().
  void ClearCreatedProxies();

//BTX
protected:
  vtkSMStateLoaderBase();
  ~vtkSMStateLoaderBase();

  // Description:
  // This method is called to load a proxy state. The implementation
  // here merely calls proxy->LoadState() however, subclass can override to do
  // some state pre-processing.
  virtual int LoadProxyState(vtkPVXMLElement* proxyElement, vtkSMProxy* proxy);

  // Description:
  // If UseExistingProxies, this returns a proxy with the given SelfID if any,
  // otherwise 0.
  vtkSMProxy* GetExistingProxy(int id);

  // Description:
  // Returns a proxy from the internal CreatedProxies map with the given key,
  // if any.
  vtkSMProxy* GetCreatedProxy(int id);

  // Description:
  // Return the xml element for the state of the proxy with the given id.
  // This is used by NewProxy() when the proxy with the given id
  // is not located in the internal CreatedProxies map.
  virtual vtkPVXMLElement* LocateProxyElement(int vtkNotUsed(id))
    { return 0; }

  // Description:
  // Called after a new proxy is created.
  virtual void CreatedNewProxy(int id, vtkSMProxy* proxy)=0;

  // Description:
  // Default implementation simply requests the proxy manager
  // to create a new proxy of the given type.
  virtual vtkSMProxy* NewProxyInternal(const char* xmlgroup, 
    const char* xmlname);

  // Connection on which the state is being loaded.
  vtkIdType ConnectionID;

  // When set to true, the \c id in NewProxy() or NewProxyFromElement() is treated
  // as the SelfID for the proxy, and the loader will try to the locate the
  // proxy with the ID on the interpretor. If one exists, a new one won't be
  // created. If a new proxy is created, we set the SelfID to be the same
  // as the \c id.
  // When set to false (default), \c id is treated merely as an identifier for the proxy,
  // having no relation with its SelfID.
  bool UseExistingProxies;

  // When loading proxy state, should the revival state be loaded or general
  // state be loaded.
  int ReviveProxies;
private:
  vtkSMStateLoaderBase(const vtkSMStateLoaderBase&); // Not implemented
  void operator=(const vtkSMStateLoaderBase&); // Not implemented

  class vtkInternal;
  vtkInternal *Internal;
//ETX
};

#endif

