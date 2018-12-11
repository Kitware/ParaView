/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDeserializerProtobuf.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMDeserializerProtobuf
 * @brief   deserializes proxies from their Protobuf states.
 *
 * vtkSMDeserializerProtobuf is used to deserialize proxies from their Protobuf
 * states. This is the base class of deserialization classes that load Protobuf
 * messagess to restore proxy/servermanager state (or part thereof).
*/

#ifndef vtkSMDeserializerProtobuf_h
#define vtkSMDeserializerProtobuf_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDeserializer.h"

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMStateLocator;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMDeserializerProtobuf : public vtkSMDeserializer
{
public:
  static vtkSMDeserializerProtobuf* New();
  vtkTypeMacro(vtkSMDeserializerProtobuf, vtkSMDeserializer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the session.
   */
  vtkGetObjectMacro(StateLocator, vtkSMStateLocator);
  virtual void SetStateLocator(vtkSMStateLocator*);
  //@}

protected:
  vtkSMDeserializerProtobuf();
  ~vtkSMDeserializerProtobuf() override;

  // Friend to access NewProxy().
  friend class vtkSMProxyLocator;

  /**
   * First ask the session, to find the given proxy.
   * If not found in the session then Create a new proxy with the id if possible.
   */
  vtkSMProxy* NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator) override;

  vtkSMStateLocator* StateLocator;

private:
  vtkSMDeserializerProtobuf(const vtkSMDeserializerProtobuf&) = delete;
  void operator=(const vtkSMDeserializerProtobuf&) = delete;
};

#endif
