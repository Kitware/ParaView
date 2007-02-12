/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPluginLoader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPluginLoader - Object that can
// be used to load plugins in the server manager.
// .SECTION Description
// vtkSMPluginLoader can be used to load plugins into the server manager.

#ifndef __vtkSMPluginLoader_h
#define __vtkSMPluginLoader_h

#include "vtkSMObject.h"

class VTK_EXPORT vtkSMPluginLoader : public vtkSMObject
{
public:
  static vtkSMPluginLoader* New();
  vtkTypeRevisionMacro(vtkSMPluginLoader, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // set the filename and load the plugin
  void SetFileName(const char* file);
  vtkGetStringMacro(FileName);
  
  // Description:
  // Get the Server Manager XML from a loaded plugin
  vtkGetStringMacro(ServerManagerXML);

  // Desctiption:
  // Get whether the plugin is loaded
  vtkGetMacro(Loaded, int);

protected:
  vtkSMPluginLoader();
  ~vtkSMPluginLoader();

  char* FileName;
  int Loaded;
  char* ServerManagerXML;

private:
  vtkSMPluginLoader(const vtkSMPluginLoader&); // Not implemented.
  void operator=(const vtkSMPluginLoader&); // Not implemented.
};


#endif

