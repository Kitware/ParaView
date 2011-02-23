/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMProxyProperty
// .SECTION Description
//

#ifndef __vtkPMProxyProperty_h
#define __vtkPMProxyProperty_h

#include "vtkPMProperty.h"

class VTK_EXPORT vtkPMProxyProperty : public vtkPMProperty
{
public:
  static vtkPMProxyProperty* New();
  vtkTypeMacro(vtkPMProxyProperty, vtkPMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Command that can be used to remove inputs. If set, this
  // command is called before the main Command is called with
  // all the arguments.
  vtkGetStringMacro(CleanCommand);

  // Description:
  // Remove command is the command called to remove the VTK
  // object on the server-side. If set, CleanCommand is ignored.
  // Instead for every proxy that was absent from the proxies
  // previously pushed, the RemoveCommand is invoked.
  vtkGetStringMacro(RemoveCommand);

  // When set to true, the property will push a NULL i.e. 0 when there are no
  // proxies in the property. Not used when CleanCommand or RemoveCommand is
  // set. Default is false.
  vtkGetMacro(NullOnEmpty, bool);

//BTX
protected:
  vtkPMProxyProperty();
  ~vtkPMProxyProperty();

  // Description:
  // Push a new state to the underneath implementation
  virtual bool Push(vtkSMMessage*, int);

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

  // Description:
  // Parse the xml for the property.
  virtual bool ReadXMLAttributes(vtkPMProxy* proxyhelper, vtkPVXMLElement* element);

  // Description:
  // Command that can be used to remove inputs. If set, this
  // command is called before the main Command is called with
  // all the arguments.
  vtkSetStringMacro(CleanCommand);
  char* CleanCommand;

  // Description:
  // Remove command is the command called to remove the VTK
  // object on the server-side. If set, CleanCommand is ignored.
  // Instead for every proxy that was absent from the proxies
  // previously pushed, the RemoveCommand is invoked.
  vtkSetStringMacro(RemoveCommand);
  char* RemoveCommand;

  // When set to true, the property will push a NULL i.e. 0 when there are no
  // proxies in the property. Not used when CleanCommand or RemoveCommand is
  // set. Default is false.
  vtkSetMacro(NullOnEmpty, bool);
  int NullOnEmpty;


  enum TypeArg {
    VTK, SMProxy, Kernel
    };

  // Proxy type: VTK (default), SMProxy, Kernel,
  // In the XML we expect argument_type="VTK"     (default value if not set)
  //                      argument_type="SMProxy"
  //                      argument_type="Kernel"
  TypeArg ArgumentType;

  // Base on the ArgumentType will return either the VTK object or the SMProxy object
  vtkObjectBase* GetObjectBase(vtkTypeUInt32 globalId);

  // Allow to detect if a null argument is really meant to be null
  bool IsValidNull(vtkTypeUInt32 globalId);

private:
  vtkPMProxyProperty(const vtkPMProxyProperty&); // Not implemented
  void operator=(const vtkPMProxyProperty&); // Not implemented
  class InternalCache;
  InternalCache *Cache;
//ETX
};

#endif
