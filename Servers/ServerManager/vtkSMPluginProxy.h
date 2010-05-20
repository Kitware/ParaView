/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPluginProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPluginProxy - proxy for vtkPVPluginLoader
// .SECTION Description

#ifndef __vtkSMPluginProxy_h
#define __vtkSMPluginProxy_h

#include "vtkSMProxy.h"
class vtkPVPluginInformation;
class vtkStringArray;
class vtkIntArray;
class vtkAbstractArray;
class vtkSMProperty;
class vtkClientServerStream;

class VTK_EXPORT vtkSMPluginProxy : public vtkSMProxy
{
public:
  static vtkSMPluginProxy* New();
  vtkTypeMacro(vtkSMPluginProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Calling server PVPluginLoader to load the given plugin. 
  // return 1 on success; 0 on failure
  virtual vtkPVPluginInformation* Load(const char* filename);
  
  // Description:
  // Get the plugin information object
  vtkGetObjectMacro(PluginInfo, vtkPVPluginInformation);
  
protected:
  vtkSMPluginProxy();
  ~vtkSMPluginProxy();

private:
  vtkSMPluginProxy(const vtkSMPluginProxy&); // Not implemented
  void operator=(const vtkSMPluginProxy&); // Not implemented
  
  vtkPVPluginInformation* PluginInfo;
};

#endif

