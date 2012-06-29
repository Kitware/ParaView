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
// .NAME vtkSMPluginLoaderProxy - used to load a plugin remotely.
// .SECTION Description
// vtkSMPluginLoaderProxy is used to load a plugin on dataserver and
// renderserver processes. Simply call vtkSMPluginLoaderProxy::LoadPlugin() with
// the right path to load the plugin remotely.

#ifndef __vtkSMPluginLoaderProxy_h
#define __vtkSMPluginLoaderProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMPluginLoaderProxy : public vtkSMProxy
{
public:
  static vtkSMPluginLoaderProxy* New();
  vtkTypeMacro(vtkSMPluginLoaderProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Loads the plugin. Returns true on success else false. To get the error
  // string, call UpdatePropertyInformation() on this proxy and then look at the
  // ErrorString property.
  bool LoadPlugin(const char* filename);

  // Description:
  // Loads the configuration xml contents. Look at
  // vtkPVPluginTracker::LoadPluginConfigurationXMLFromString() to see the
  // details about the configuration xml.
  void LoadPluginConfigurationXMLFromString(const char* xmlcontents);

//BTX
protected:
  vtkSMPluginLoaderProxy();
  ~vtkSMPluginLoaderProxy();

private:
  vtkSMPluginLoaderProxy(const vtkSMPluginLoaderProxy&); // Not implemented
  void operator=(const vtkSMPluginLoaderProxy&); // Not implemented
//ETX
};

#endif
