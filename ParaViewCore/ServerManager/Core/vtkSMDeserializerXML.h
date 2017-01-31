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
/**
 * @class   vtkSMDeserializerXML
 * @brief   deserializes proxies from their XML states.
 *
 * vtkSMDeserializer is used to deserialize proxies from their XML states. This
 * is the base class of deserialization classes that load XMLs to restore
 * servermanager state (or part thereof).
*/

#ifndef vtkSMDeserializerXML_h
#define vtkSMDeserializerXML_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDeserializer.h"

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMDeserializerXML : public vtkSMDeserializer
{
public:
  static vtkSMDeserializerXML* New();
  vtkTypeMacro(vtkSMDeserializerXML, vtkSMDeserializer);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSMDeserializerXML();
  ~vtkSMDeserializerXML();

  // Friend to access NewProxy().
  friend class vtkSMProxyLocator;

  /**
   * Create a new proxy with the \c id if possible.
   */
  virtual vtkSMProxy* NewProxy(vtkTypeUInt32 id, vtkSMProxyLocator* locator) VTK_OVERRIDE;

  /**
   * Locate the XML for the proxy with the given id.
   */
  virtual vtkPVXMLElement* LocateProxyElement(vtkTypeUInt32 id);

  /**
   * TEMPORARY. Used to load the state on the proxy. This is only for the sake
   * of the lookmark state loader until we get the chance to clean it up.
   * DONT override this method.
   */
  virtual int LoadProxyState(vtkPVXMLElement* element, vtkSMProxy*, vtkSMProxyLocator* locator);

  /**
   * Create a new proxy of the given group and name. Default implementation
   * simply asks the proxy manager to create a new proxy of the requested type.
   */
  virtual vtkSMProxy* CreateProxy(
    const char* xmlgroup, const char* xmlname, const char* subProxyName = NULL) VTK_OVERRIDE;

  /**
   * Called after a new proxy has been created. Gives the subclasses an
   * opportunity to perform certain tasks such as registering proxies etc.
   * Default implementation is empty.
   */
  virtual void CreatedNewProxy(vtkTypeUInt32 id, vtkSMProxy* proxy);

private:
  vtkSMDeserializerXML(const vtkSMDeserializerXML&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMDeserializerXML&) VTK_DELETE_FUNCTION;
};

#endif
