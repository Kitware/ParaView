/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDeserializer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDeserializer - deserializes proxies from their states.
// .SECTION Description
// vtkSMDeserializer is used to deserialize proxies from their XML/Protobuf/?
// states. This is the base class of deserialization classes that load
// XMLs/Protobuf/? to restore servermanager state (or part thereof).

#ifndef __vtkSMDeserializer_h
#define __vtkSMDeserializer_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMSession;
class vtkSMSessionProxyManager;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMDeserializer : public vtkObject
{
public:
  vtkTypeMacro(vtkSMDeserializer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience method for setting the SessionProxyManager. This is equivalent
  // to calling
  // vtkSMDeserializer::SetSessionProxyManager(session->GetSessionProxyManager()).
  void SetSession(vtkSMSession* session);

  // Description:
  // Get/Set the proxy manager on which this deserializer is expected to
  // operate.
  vtkSMSessionProxyManager* GetSessionProxyManager();
  void SetSessionProxyManager(vtkSMSessionProxyManager*);

  // Description:
  // Provides access to the session. This is same as calling
  // this->GetSessionProxyManager()->GetSession() (with NULL checks).
  vtkSMSession* GetSession();

//BTX
protected:
  vtkSMDeserializer();
  ~vtkSMDeserializer();

  // Friend to access NewProxy().
  friend class vtkSMProxyLocator;

  // Description:
  // Create a new proxy with the id if possible.
  virtual vtkSMProxy* NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator) = 0;

  // Description:
  // Create a new proxy of the given group and name. Default implementation
  // simply asks the proxy manager to create a new proxy of the requested type.
  virtual vtkSMProxy* CreateProxy(const char* xmlgroup, const char* xmlname,
                                  const char* subProxyName = NULL);

  vtkWeakPointer<vtkSMSessionProxyManager> SessionProxyManager;

private:
  vtkSMDeserializer(const vtkSMDeserializer&); // Not implemented
  void operator=(const vtkSMDeserializer&); // Not implemented
//ETX
};

#endif
