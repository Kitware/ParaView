/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPluginInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPluginInformation - SM Plugin information object.
// .SECTION Description

#ifndef __vtkPVPluginInformation_h
#define __vtkPVPluginInformation_h

#include "vtkPVInformation.h"

class vtkClientServerStream;

class VTK_EXPORT vtkPVPluginInformation : public vtkPVInformation
{
public:
  static vtkPVPluginInformation* New();
  vtkTypeMacro(vtkPVPluginInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get SMPlugin's PluginName/FileName/SearchPaths/Error
  vtkSetStringMacro(PluginName);
  vtkGetStringMacro(PluginName);
  
  // Description:
  // Set/Get SMPlugin's Version string
  vtkSetStringMacro(PluginVersion);
  vtkGetStringMacro(PluginVersion);
  
  // Description:
  // Set/Get SMPlugin's FileName (fullpath to the module)
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  
  // Description:
  // Get/Set whether the plugin is loaded
  vtkGetMacro(Loaded, int);
  vtkSetMacro(Loaded, int);
  
  // Description:
  // Get/Set whether the plugin shoudld be auto-loaded
  vtkGetMacro(AutoLoad, int);
  vtkSetMacro(AutoLoad, int);
  vtkBooleanMacro(AutoLoad, int);
  
  // Description:
  // Get/Set whether the plugin is required on server
  vtkGetMacro(RequiredOnServer, int);
  vtkSetMacro(RequiredOnServer, int);
  vtkBooleanMacro(RequiredOnServer, int);
  
  // Description:
  // Get/Set whether the plugin is required on client
  vtkGetMacro(RequiredOnClient, int);
  vtkSetMacro(RequiredOnClient, int);
  vtkBooleanMacro(RequiredOnClient, int);
  
  // Description:
  // Get/Set the Error string if the plugin failed to load
  vtkSetStringMacro(Error);
  vtkGetStringMacro(Error);
  
  // Description:
  // Get/Set the ServerURI string if the plugin failed to load
  vtkSetStringMacro(ServerURI);
  vtkGetStringMacro(ServerURI);
  
  // Description:
  // Get/Set the plugin names that this plugin depends on
  vtkSetStringMacro(RequiredPlugins);
  vtkGetStringMacro(RequiredPlugins);
  
  // Description:
  // Get a string of standard search paths (path1;path2;path3)
  // search paths are based on PV_PLUGIN_PATH,
  // plugin dir relative to executable.
  vtkSetStringMacro(SearchPaths);
  vtkGetStringMacro(SearchPaths);

  // Description:
  // Returns 1 if the plugin info pointing to the same plugin.
  int Compare(vtkPVPluginInformation *info);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Make a deep copy of the info object.
  void DeepCopy(vtkPVPluginInformation *info);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);
  
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

protected:
  vtkPVPluginInformation();
  ~vtkPVPluginInformation();

  // Description:
  // Remove all information. 
  void ClearInfo();
  
  // Check and compare string
  // return true if the two string are not null and equal; else, return 0;
  bool CompareInfoString(const char* str1, const char* str2);
  
  char *PluginName;
  char *FileName;
  char* Error;
  char* SearchPaths;
  char* PluginVersion;
  char* ServerURI;
  char* RequiredPlugins;
  
  int Loaded;
  int AutoLoad;
  int RequiredOnClient;
  int RequiredOnServer;

  vtkPVPluginInformation(const vtkPVPluginInformation&); // Not implemented
  void operator=(const vtkPVPluginInformation&); // Not implemented
};

#endif
