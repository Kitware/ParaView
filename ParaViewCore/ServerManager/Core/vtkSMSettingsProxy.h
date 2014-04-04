/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSettingsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSettingsProxy - proxy subclass responsible for linking settings
// proxies and settings classes on the VTK side.
// .SECTION Description
// vtkSMSettingsProxy is used as a proxy for settings objects. It listens
// for changes to the underlying VTK objects and updates the proxy properties
// whenever the VTK object settings change.

#ifndef __vtkSMSettingsProxy_h
#define __vtkSMSettingsProxy_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkSMSettingsObserver;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMSettingsProxy : public vtkSMProxy
{
public:
  static vtkSMSettingsProxy* New();
  vtkTypeMacro(vtkSMSettingsProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to link information properties with their corresponding
  // "setter" properties.
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element);

//BTX
 protected:
  vtkSMSettingsProxy();
  ~vtkSMSettingsProxy();

  // Description:
  // Overridden from vtkSMProxy to install an observer on the VTK object
  virtual void CreateVTKObjects();

  friend class vtkSMSettingsObserver;

  // Description:
  // Called whenever the VTK object is modified
  void ExecuteEvent(unsigned long eventId);

  vtkSMSettingsObserver* Observer;

private:
  vtkSMSettingsProxy(const vtkSMSettingsProxy&); // Not implemented
  void operator=(const vtkSMSettingsProxy&); // Not implemented
//ETX
};

#endif
