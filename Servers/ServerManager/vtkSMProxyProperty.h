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
  void AddProxy(vtkSMProxy* proxy);

  // Description:
  // Add a proxy to the list of proxies without calling Modified
  // (if modify is false). This is commonly used when ImmediateUpdate
  // is true but it is more efficient to avoid calling Update until
  // the last proxy is added. To do this, add all proxies with modify=false
  // and call Modified after the last.
  void AddProxy(vtkSMProxy* proxy, int modify);

  // Description:
  // Remove all proxies from the list.
  void RemoveAllProxies();

  // Description:
  // Returns the number of proxies.
  unsigned int GetNumberOfProxies();

  // Description:
  // Return a proxy. No bounds check is performed.
  vtkSMProxy* GetProxy(unsigned int idx);

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
  virtual void AppendCommandToStream(
    vtkSMProxy*, vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  // Description:
  // Update all proxies referred by this property.
  virtual void UpdateAllInputs();

  // Description:
  // Saves the state of the object in XML format. 
  virtual void SaveState(const char* name,  ofstream* file, vtkIndent indent);

  // Description:
  void AddPreviousProxy(vtkSMProxy* proxy);

  // Description:
  void ClearPreviousProxies();

  // Description:
  void RemoveConsumers(vtkSMProxy* proxy);

  vtkSMProxyPropertyInternals* PPInternals;

private:
  vtkSMProxyProperty(const vtkSMProxyProperty&); // Not implemented
  void operator=(const vtkSMProxyProperty&); // Not implemented
};

#endif
