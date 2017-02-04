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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the session.
   */
  vtkGetObjectMacro(StateLocator, vtkSMStateLocator);
  virtual void SetStateLocator(vtkSMStateLocator*);
  //@}

protected:
  vtkSMDeserializerProtobuf();
  ~vtkSMDeserializerProtobuf();

  // Friend to access NewProxy().
  friend class vtkSMProxyLocator;

  /**
   * First ask the session, to find the given proxy.
   * If not found in the session then Create a new proxy with the id if possible.
   */
  virtual vtkSMProxy* NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator) VTK_OVERRIDE;

  vtkSMStateLocator* StateLocator;

private:
  vtkSMDeserializerProtobuf(const vtkSMDeserializerProtobuf&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMDeserializerProtobuf&) VTK_DELETE_FUNCTION;
};

#endif
