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

class vtkStringArray;

class VTK_EXPORT vtkPVPluginLoader : public vtkObject
{
public:
  static vtkPVPluginLoader* New();
  vtkTypeRevisionMacro(vtkPVPluginLoader, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // set the filename and load the plugin
  void SetFileName(const char* file);
  vtkGetStringMacro(FileName);
  
  // Description:
  // Get the Server Manager XML from a loaded plugin
  // the string array contains chunks of XML to process
  vtkGetObjectMacro(ServerManagerXML, vtkStringArray);

  // Desctiption:
  // Get whether the plugin is loaded
  vtkGetMacro(Loaded, int);
  
  // Desctiption:
  // Get the error string if the plugin failed to load
  vtkGetStringMacro(Error);
  
  // Desctiption:
  // Get a string of standard search paths (path1;path2;path3)
  // search paths are based on PV_PLUGIN_PATH,
  // plugin dir relative to executable.
  vtkGetStringMacro(SearchPaths);

protected:
  vtkPVPluginLoader();
  ~vtkPVPluginLoader();
  
  vtkSetStringMacro(Error);

  char* FileName;
  int Loaded;
  vtkStringArray* ServerManagerXML;
  char* Error;
  char* SearchPaths;

private:
  vtkPVPluginLoader(const vtkPVPluginLoader&); // Not implemented.
  void operator=(const vtkPVPluginLoader&); // Not implemented.
};


#endif

