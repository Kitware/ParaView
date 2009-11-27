/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPluginLoader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPluginLoader - Object that can
// be used to load plugins in the server manager.
// .SECTION Description
// vtkPVPluginLoader can be used to load plugins into the server manager.

#ifndef __vtkPVPluginLoader_h
#define __vtkPVPluginLoader_h

#include "vtkObject.h"

class vtkIntArray;
class vtkPVPlugin;
class vtkPVPluginInformation;
class vtkStringArray;

class VTK_EXPORT vtkPVPluginLoader : public vtkObject
{
public:
  static vtkPVPluginLoader* New();
  vtkTypeRevisionMacro(vtkPVPluginLoader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // set/get the filename and load the plugin
  void SetFileName(const char* file);
  const char* GetFileName();

//BTX
  // Description:
  // Loads the paraview plugin.
  void Load(vtkPVPlugin*);
//ETX
  
  // Description:
  // Get the Server Manager PluginInformation object
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
    
  // Description:
  // Get the plugin name string
  const char* GetPluginName();
    
  // Description:
  // Get the plugin version string
  const char* GetPluginVersion();
  
  // Description:
  // Get whether the plugin is loaded
  int GetLoaded(); 
   
  // Description:
  // Get the error string if the plugin failed to load
  const char* GetError();
  
  // Description:
  // Get a string of standard search paths (path1;path2;path3)
  // search paths are based on PV_PLUGIN_PATH,
  // plugin dir relative to executable.
  const char* GetSearchPaths();

protected:
  vtkPVPluginLoader();
  ~vtkPVPluginLoader();
  
  vtkPVPluginInformation* PluginInfo;

  vtkStringArray* ServerManagerXML;
  vtkStringArray* PythonModuleNames;
  vtkStringArray* PythonModuleSources;
  vtkIntArray*    PythonPackageFlags;
  bool DebugPlugin;

private:
  vtkPVPluginLoader(const vtkPVPluginLoader&); // Not implemented.
  void operator=(const vtkPVPluginLoader&); // Not implemented.
};


#endif

