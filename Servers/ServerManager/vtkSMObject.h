/*=========================================================================

  Program:   ParaView
  Module:    vtkSMObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMObject - superclass for most server manager classes
// .SECTION Description
// vtkSMObject provides several methods common to most server manager
// classes. These are mostly for setting and getting singletons including
// the communication and process modules and the proxy manager.

#ifndef __vtkSMObject_h
#define __vtkSMObject_h

#include "vtkObject.h"

class vtkSMCommunicationModule;
class vtkSMProcessModule;
class vtkSMProxyManager;

class VTK_EXPORT vtkSMObject : public vtkObject
{
public:
  static vtkSMObject* New();
  vtkTypeRevisionMacro(vtkSMObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // The communication module singleton is used by server manager
  // objects to communicate with servers.
  static vtkSMCommunicationModule* GetCommunicationModule();
  static void SetCommunicationModule(vtkSMCommunicationModule* cm);

  // Description:
  // The process module singleton is used by server manager objects
  // to process/execute streams.
  static vtkSMProcessModule* GetProcessModule();
  static void SetProcessModule(vtkSMProcessModule* pm);

  // Description:
  // Proxy manager singleton stores all proxy groups and instances.
  static vtkSMProxyManager* GetProxyManager();
  static void SetProxyManager(vtkSMProxyManager* pm);

protected:
  vtkSMObject();
  ~vtkSMObject();

  static vtkSMCommunicationModule* CommunicationModule;
  static vtkSMProcessModule* ProcessModule;
  static vtkSMProxyManager* ProxyManager;

private:
  vtkSMObject(const vtkSMObject&); // Not implemented
  void operator=(const vtkSMObject&); // Not implemented
};

#endif
