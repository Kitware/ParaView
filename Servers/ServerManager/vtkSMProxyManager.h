/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyManager -
// .SECTION Description

#ifndef __vtkSMProxyManager_h
#define __vtkSMProxyManager_h

#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProperty;
class vtkSMProxy;
class vtkSMProxyManagerInternals;

class VTK_EXPORT vtkSMProxyManager : public vtkSMObject
{
public:
  static vtkSMProxyManager* New();
  vtkTypeRevisionMacro(vtkSMProxyManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  vtkSMProxy* NewProxy(const char* groupName, const char* proxyName);

  // Description:
  int DeleteProxy(vtkSMProxy* proxy);

protected:
  vtkSMProxyManager();
  ~vtkSMProxyManager();

  void AddElement(
    const char* groupName, const char* name, vtkPVXMLElement* element);

  vtkSMProperty* NewProperty(vtkPVXMLElement* pelement);
  vtkSMProxy* NewProxy(vtkPVXMLElement* element);

//BTX
  friend class vtkSMXMLParser;
//ETX

private:
  vtkSMProxyManagerInternals* Internals;

private:
  vtkSMProxyManager(const vtkSMProxyManager&); // Not implemented
  void operator=(const vtkSMProxyManager&); // Not implemented
};

#endif
