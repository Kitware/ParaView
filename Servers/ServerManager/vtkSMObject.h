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
// .NAME vtkSMObject -
// .SECTION Description

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
  static vtkSMCommunicationModule* GetCommunicationModule();
  static void SetCommunicationModule(vtkSMCommunicationModule* cm);

  // Description:
  static vtkSMProcessModule* GetProcessModule();
  static void SetProcessModule(vtkSMProcessModule* pm);

  // Description:
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
