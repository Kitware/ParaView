/*=========================================================================

  Program:   ParaView
  Module:    vtkSIProxyProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIProxyProperty
 *
 * ServerSide Property use to set Object array as method argument.
 * Those object could be either SMProxy instance or their SIProxy instance
 * or the VTK object managed by the SIProxy instance. The type of object is
 * specified inside the XML definition of the property with the attribute
 * argument_type=[VTK, SMProxy, SIProxy].
*/

#ifndef vtkSIProxyProperty_h
#define vtkSIProxyProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIProxyProperty : public vtkSIProperty
{
public:
  static vtkSIProxyProperty* New();
  vtkTypeMacro(vtkSIProxyProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Command that can be used to remove inputs. If set, this
   * command is called before the main Command is called with
   * all the arguments.
   */
  vtkGetStringMacro(CleanCommand);
  //@}

  //@{
  /**
   * Remove command is the command called to remove the VTK
   * object on the server-side. If set, CleanCommand is ignored.
   * Instead for every proxy that was absent from the proxies
   * previously pushed, the RemoveCommand is invoked.
   */
  vtkGetStringMacro(RemoveCommand);
  //@}

  // When set to true, the property will push a NULL i.e. 0 when there are no
  // proxies in the property. Not used when CleanCommand or RemoveCommand is
  // set. Default is false.
  vtkGetMacro(NullOnEmpty, bool);

protected:
  vtkSIProxyProperty();
  ~vtkSIProxyProperty() override;

  /**
   * Push a new state to the underneath implementation
   */
  bool Push(vtkSMMessage*, int) override;

  /**
   * Parse the xml for the property.
   */
  bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) override;

  //@{
  /**
   * Command that can be used to remove inputs. If set, this
   * command is called before the main Command is called with
   * all the arguments.
   */
  vtkSetStringMacro(CleanCommand);
  char* CleanCommand;
  //@}

  //@{
  /**
   * Remove command is the command called to remove the VTK
   * object on the server-side. If set, CleanCommand is ignored.
   * Instead for every proxy that was absent from the proxies
   * previously pushed, the RemoveCommand is invoked.
   */
  vtkSetStringMacro(RemoveCommand);
  char* RemoveCommand;
  //@}

  // When set to true, the property will push a NULL i.e. 0 when there are no
  // proxies in the property. Not used when CleanCommand or RemoveCommand is
  // set. Default is false.
  vtkSetMacro(NullOnEmpty, bool);
  bool NullOnEmpty;

  enum TypeArg
  {
    VTK,
    SMProxy,
    SIProxy
  };

  // Proxy type: VTK (default), SMProxy, Kernel,
  // In the XML we expect argument_type="VTK"     (default value if not set)
  //                      argument_type="SMProxy"
  //                      argument_type="SIProxy"
  TypeArg ArgumentType;

  // Base on the ArgumentType will return either the VTK object or the SMProxy object
  vtkObjectBase* GetObjectBase(vtkTypeUInt32 globalId);

  // Allow to detect if a null argument is really meant to be null
  bool IsValidNull(vtkTypeUInt32 globalId);

private:
  vtkSIProxyProperty(const vtkSIProxyProperty&) = delete;
  void operator=(const vtkSIProxyProperty&) = delete;
  class InternalCache;
  InternalCache* Cache;

  class vtkObjectCache;
  vtkObjectCache* ObjectCache;
};

#endif
