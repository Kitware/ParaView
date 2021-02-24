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
/**
 * @class   vtkSMDeserializer
 * @brief   deserializes proxies from their states.
 *
 * vtkSMDeserializer is used to deserialize proxies from their XML/Protobuf/?
 * states. This is the base class of deserialization classes that load
 * XMLs/Protobuf/? to restore servermanager state (or part thereof).
*/

#ifndef vtkSMDeserializer_h
#define vtkSMDeserializer_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkWeakPointer.h"                 // needed for vtkWeakPointer.

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMSession;
class vtkSMSessionProxyManager;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDeserializer : public vtkObject
{
public:
  vtkTypeMacro(vtkSMDeserializer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Convenience method for setting the SessionProxyManager. This is equivalent
   * to calling
   * vtkSMDeserializer::SetSessionProxyManager(session->GetSessionProxyManager()).
   */
  void SetSession(vtkSMSession* session);

  //@{
  /**
   * Get/Set the proxy manager on which this deserializer is expected to
   * operate.
   */
  vtkSMSessionProxyManager* GetSessionProxyManager();
  void SetSessionProxyManager(vtkSMSessionProxyManager*);
  //@}

  /**
   * Provides access to the session. This is same as calling
   * this->GetSessionProxyManager()->GetSession() (with nullptr checks).
   */
  vtkSMSession* GetSession();

protected:
  vtkSMDeserializer();
  ~vtkSMDeserializer() override;

  // Friend to access NewProxy().
  friend class vtkSMProxyLocator;

  /**
   * Create a new proxy with the id if possible.
   */
  virtual vtkSMProxy* NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator) = 0;

  /**
   * Create a new proxy of the given group and name. Default implementation
   * simply asks the proxy manager to create a new proxy of the requested type.
   */
  virtual vtkSMProxy* CreateProxy(
    const char* xmlgroup, const char* xmlname, const char* subProxyName = nullptr);

  vtkWeakPointer<vtkSMSessionProxyManager> SessionProxyManager;

private:
  vtkSMDeserializer(const vtkSMDeserializer&) = delete;
  void operator=(const vtkSMDeserializer&) = delete;
};

#endif
