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
// .NAME vtkPVPluginsInformation - information about plugins tracked by
// vtkPVPluginTracker.
// .SECTION Description
// vtkPVPluginsInformation is used to collect information about plugins tracked
// by vtkPVPluginTracker.

#ifndef __vtkPVPluginsInformation_h
#define __vtkPVPluginsInformation_h

#include "vtkPVInformation.h"

class VTK_EXPORT vtkPVPluginsInformation : public vtkPVInformation
{
public:
  static vtkPVPluginsInformation* New();
  vtkTypeMacro(vtkPVPluginsInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // API to iterate over the information collected for each plugin.
  unsigned int GetNumberOfPlugins();
  const char* GetPluginName(unsigned int);
  const char* GetPluginFileName(unsigned int);
  const char* GetPluginVersion(unsigned int);
  bool GetPluginLoaded(unsigned int);
  const char* GetRequiredPlugins(unsigned int);
  bool GetRequiredOnServer(unsigned int);
  bool GetRequiredOnClient(unsigned int);
  bool GetAutoLoad(unsigned int);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  //BTX
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  //ETX

  // Description:
  // Get the plugin search path.
  vtkGetStringMacro(SearchPaths);
//BTX
protected:
  vtkPVPluginsInformation();
  ~vtkPVPluginsInformation();

  char* SearchPaths;
  vtkSetStringMacro(SearchPaths);

private:
  vtkPVPluginsInformation(const vtkPVPluginsInformation&); // Not implemented
  void operator=(const vtkPVPluginsInformation&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
