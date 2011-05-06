/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPluginsInformation.h

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
  // Note that unlike other properties, this one is updated as a consequence of
  // calling PluginRequirementsSatisfied().
  const char* GetPluginStatusMessage(unsigned int);

  // Description:
  // API to change auto-load status.
  void SetAutoLoad(unsigned int cc, bool);

  // Description:
  // This is a hack. When the user sets an auto-load option from  the GUI to
  // avoid that choice being overwritten as the information object is updated
  // over time as new plugins are loaded/unloaded, the pqPluginDialog uses this
  // method to set the auto-load flag. This flag is not communicated across
  // processes, but when called, GetAutoLoad() will return the value set using
  // this method.
  void SetAutoLoadAndForce(unsigned int cc, bool);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Updates the local information with elements from other without overriding
  // auto-load state.
  void Update(vtkPVPluginsInformation* other);

  //BTX
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  //ETX

  // Description:
  // Get the plugin search path.
  vtkGetStringMacro(SearchPaths);

  // Description:
  // Method to validate if the plugin requirements are met across processes.
  // This also updated the "StatusMessage" for all the plugins. If StatusMessage
  // is empty for a loaded plugin, it implies that everything is fine. If some
  // requirement is not met, the StatusMessage includes the error message.
  static bool PluginRequirementsSatisfied(
    vtkPVPluginsInformation* client_plugins,
    vtkPVPluginsInformation* server_plugins);
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
