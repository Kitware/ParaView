/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDeserializerXML.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDeserializerXML - deserializes proxies from their XML states.
// .SECTION Description
// vtkSMDeserializer is used to deserialize proxies from their XML states. This
// is the base class of deserialization classes that load XMLs to restore
// servermanager state (or part thereof).

#ifndef __vtkSMDeserializerXML_h
#define __vtkSMDeserializerXML_h

#include "vtkSMDeserializer.h"

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;

class VTK_EXPORT vtkSMDeserializerXML : public vtkSMDeserializer
{
public:
  static vtkSMDeserializerXML* New();
  vtkTypeMacro(vtkSMDeserializerXML, vtkSMDeserializer);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMDeserializerXML();
  ~vtkSMDeserializerXML();

  // Friend to access NewProxy().
  friend class vtkSMProxyLocator;

  // Description:
  // Create a new proxy with the \c id if possible.
  virtual vtkSMProxy* NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator);

  // Description:
  // Locate the XML for the proxy with the given id.
  virtual vtkPVXMLElement* LocateProxyElement(vtkTypeUInt32 id);

  // Description:
  // TEMPORARY. Used to load the state on the proxy. This is only for the sake
  // of the lookmark state loader until we get the chance to clean it up.
  // DONT override this method.
  virtual int LoadProxyState(vtkPVXMLElement* element, vtkSMProxy*,
                             vtkSMProxyLocator* locator);

  // Description:
  // Create a new proxy of the given group and name. Default implementation
  // simply asks the proxy manager to create a new proxy of the requested type.
  virtual vtkSMProxy* CreateProxy(const char* xmlgroup, const char* xmlname,
                                  const char* subProxyName = NULL);

  // Description:
  // Called after a new proxy has been created. Gives the subclasses an
  // opportunity to perform certain tasks such as registering proxies etc.
  // Default implementation is empty.
  virtual void CreatedNewProxy(vtkTypeUInt32 id, vtkSMProxy* proxy);

private:
  vtkSMDeserializerXML(const vtkSMDeserializerXML&); // Not implemented
  void operator=(const vtkSMDeserializerXML&); // Not implemented
//ETX
};

#endif
