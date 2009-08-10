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
  vtkTypeRevisionMacro(vtkSMPluginProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates all property informations by calling UpdateInformation()
  // and populating the values. It also calls UpdateDependentDomains()
  // on all properties to make sure that domains that depend on the
  // information are updated.
  virtual void UpdatePropertyInformation();
  virtual void UpdatePropertyInformation(vtkSMProperty* prop)
    { this->Superclass::UpdatePropertyInformation(prop); }

  // Description:
  // Calling server PVPluginLoader to load the given plugin. 
  // return 1 on success; 0 on failure
  virtual vtkPVPluginInformation* Load(const char* filename);
  
  // Description:
  // Get the plugin information object
  vtkGetObjectMacro(PluginInfo, vtkPVPluginInformation);
  
  // Description:
  // Get the Server Manager XML from a loaded plugin
  // the string array contains chunks of XML to process
  vtkGetObjectMacro(ServerManagerXML, vtkStringArray);

  // Description:
  // Get a list of Python modules from a loaded plugin.  The string array
  // contains full names of Python modules.
  vtkGetObjectMacro(PythonModuleNames, vtkStringArray);

  // Description:
  // Get the Python source for the Python modules in a loaded plugin.  The
  // string array contains the Python source for a given module.  The entries in
  // this array correspond to the entries of the same index in the
  // PythonModuleNames array.
  vtkGetObjectMacro(PythonModuleSources, vtkStringArray);

  // Description:
  // Get a list of flags specifying whether a given module is really a package.
  // A 1 for package, 0 for module.  The entries in this array correspond to the
  // entries of the same index in the PythonModuleNames and PythonModuleSources
  // arrays.
  vtkGetObjectMacro(PythonPackageFlags, vtkIntArray);
    
protected:
  vtkSMPluginProxy();
  ~vtkSMPluginProxy();

private:
  vtkSMPluginProxy(const vtkSMPluginProxy&); // Not implemented
  void operator=(const vtkSMPluginProxy&); // Not implemented
  
  vtkPVPluginInformation* PluginInfo;
  
  vtkStringArray* ServerManagerXML;
  vtkStringArray* PythonModuleNames;
  vtkStringArray* PythonModuleSources;
  vtkIntArray*    PythonPackageFlags;
  
  //return 1 on success, and 0 on failure
  int GetPropertyArray(vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop,
    vtkAbstractArray* valuearray);
  int GetArrayStream(vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, const char* serverCmd,
    vtkClientServerStream* valuelist, 
    const char* streamobjectname, const char* streamobjectcmd);
    
};

#endif

