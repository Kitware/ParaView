/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyProperty - property representing pointer(s) to vtkObject(s)
// .SECTION Description
// vtkSMProxyProperty is a concrete sub-class of vtkSMProperty representing
// pointer(s) to vtkObject(s) (through vtkSMProxy). Note that if the proxy
// has multiple IDs, they are all appended to the command stream. If 
// UpdateSelf is true, the proxy ids (as opposed to the server object ids)
// are passed to the stream.
// .SECTION See Also
// vtkSMProperty

#ifndef __vtkSMProxyProperty_h
#define __vtkSMProxyProperty_h

#include "vtkSMProperty.h"

class vtkSMProxy;
//BTX
struct vtkSMProxyPropertyInternals;
//ETX

class VTK_EXPORT vtkSMProxyProperty : public vtkSMProperty
{
public:
  static vtkSMProxyProperty* New();
  vtkTypeRevisionMacro(vtkSMProxyProperty, vtkSMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Add a proxy to the list of proxies.
  int AddProxy(vtkSMProxy* proxy);
  int SetProxy(unsigned int idx, vtkSMProxy* proxy);

  // Description:
  // Add a proxy to the list of proxies without calling Modified
  // (if modify is false). This is commonly used when ImmediateUpdate
  // is true but it is more efficient to avoid calling Update until
  // the last proxy is added. To do this, add all proxies with modify=false
  // and call Modified after the last.
  // This will perform domain checking. If the domain check fails,
  // the proxy will not be added and 0 will be returned.
  // Returns 1 on success. If the domain check fails or the property
  // is read only, returns 0.
  // All proxies added with AddProxy() will become "consumers" of
  // the proxy passed to AppendCommandToStream().
  int AddProxy(vtkSMProxy* proxy, int modify);

  // Description:
  // Add an unchecked proxy. Does not modify the property.
  // Unchecked proxies are used by domains when verifying whether
  // a value is acceptable. To check if a value is in the domains,
  // you can do the following:
  // @verbatim
  // - RemoveAllUncheckedProxies()
  // - AddUncheckedProxy(proxy)
  // - IsInDomains()
  // @endverbatim
  void AddUncheckedProxy(vtkSMProxy* proxy);
  void SetUncheckedProxy(unsigned int idx, vtkSMProxy* proxy);

  // Description:
  // Removes all unchecked proxies.
  void RemoveAllUncheckedProxies();

  // Description:
  // Remove all proxies from the list.
  void RemoveAllProxies();

  // Description:
  // Returns the number of proxies.
  unsigned int GetNumberOfProxies();

  // Description:
  // Returns the number of unchecked proxies.
  unsigned int GetNumberOfUncheckedProxies();

  // Description:
  // Return a proxy. No bounds check is performed.
  vtkSMProxy* GetProxy(unsigned int idx);

  // Description:
  // Return a proxy. No bounds check is performed.
  vtkSMProxy* GetUncheckedProxy(unsigned int idx);

protected:
  vtkSMProxyProperty();
  ~vtkSMProxyProperty();

  //BTX
  // Description:
  // Description:
  // Append a command to update the vtk object with the property values(s).
  // The proxy objects create a stream by calling this method on all the
  // modified properties.
  // Note that if the proxy has multiple IDs, they are all appended to the 
  // command stream.  
  // All proxies added with AddProxy() will become "consumers" of
  // the proxy passed to AppendCommandToStream().
  virtual void AppendCommandToStream(
    vtkSMProxy*, vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  // Description:
  // Update all proxies referred by this property.
  virtual void UpdateAllInputs();

  // Description:
  // Saves the state of the object in XML format. 
  virtual void SaveState(const char* name,  ostream* file, vtkIndent indent);

  // Previous proxies are used by the ProxyProperty internally.
  // All proxies added with AddProxy() will become "consumers" of
  // the proxy passed to AppendCommandToStream().
  // Proxies that were added previous but that are no longer in the list
  // (removed) are no longer consumers of the proxy therefore RemoveProxy
  // is called on them. This requires keeping track of previous proxies.
  // Description:
  void AddPreviousProxy(vtkSMProxy* proxy);
  void RemoveAllPreviousProxies();

  // Description:
  // Given a proxy, remove all previous proxies from it's consumer list.
  void RemoveConsumers(vtkSMProxy* proxy);

  vtkSMProxyPropertyInternals* PPInternals;

private:
  vtkSMProxyProperty(const vtkSMProxyProperty&); // Not implemented
  void operator=(const vtkSMProxyProperty&); // Not implemented
};

#endif
