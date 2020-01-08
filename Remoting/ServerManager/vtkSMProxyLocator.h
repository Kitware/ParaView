/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyLocator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxyLocator
 * @brief   is used to locate proxies referred to in state xmls
 * while loading state files.
 *
 * vtkSMProxyLocator is used to locate proxies referred to in state xmls (and
 * otherwise) when loading state files.
*/

#ifndef vtkSMProxyLocator_h
#define vtkSMProxyLocator_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMObject.h"
#include "vtkWeakPointer.h" // needed to keep the session around

class vtkCollection;
class vtkSMDeserializer;
class vtkSMProxy;
class vtkSMSession;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMProxyLocator : public vtkSMObject
{
public:
  static vtkSMProxyLocator* New();
  vtkTypeMacro(vtkSMProxyLocator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Locate a proxy with the given "name". If none can be found returns NULL.
   * If a proxy with the name was not previously located, it will ask the
   * Deserializer (if any) to create a new proxy is possible.
   */
  virtual vtkSMProxy* LocateProxy(vtkTypeUInt32 globalID);

  //@{
  /**
   * Get/Set the de-serializer to used to locate XMLs/Protobuf for unknown proxies
   * requested to be located using LocateProxy().
   */
  void SetDeserializer(vtkSMDeserializer*);
  vtkGetObjectMacro(Deserializer, vtkSMDeserializer);
  //@}

  //@{
  /**
   * Get/Set the session.
   */
  virtual vtkSMSession* GetSession();
  virtual void SetSession(vtkSMSession* s);
  //@}

  /**
   * Clear the locator.
   */
  virtual void Clear();

  /**
   * Copy all the Located proxy into the collection.
   * This allow to keep a reference to them outside of the current locator.
   * This is needed if we don't want to delete those proxy and if we want to
   * Clear() the current ProxyLocator.
   */
  virtual void GetLocatedProxies(vtkCollection* collectionToFill);

  virtual void UseSessionToLocateProxy(bool useSessionToo)
  {
    this->LocateProxyWithSessionToo = useSessionToo;
  }

  /**
   * For custom applications managing custom state loading, one can use this
   * method to register proxies with locator to use  when LocateProxy()
   * before attempting the standard route i.e. using the deserializer or
   * session.
   */
  virtual void AssignProxy(vtkTypeUInt32 id, vtkSMProxy* proxy);

protected:
  vtkSMProxyLocator();
  ~vtkSMProxyLocator() override;

  /**
   * Create new proxy with the given id. Default implementation asks the
   * Deserializer, if any, to create a new proxy.
   */
  virtual vtkSMProxy* NewProxy(vtkTypeUInt32 globalID);

  vtkSMDeserializer* Deserializer;
  vtkWeakPointer<vtkSMSession> Session;
  bool LocateProxyWithSessionToo;

private:
  vtkSMProxyLocator(const vtkSMProxyLocator&) = delete;
  void operator=(const vtkSMProxyLocator&) = delete;

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
