/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyStatusManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPropertyStatusManager - keeps track of modified properties.
// .SECTION Description
// This class can only manage the status of Vector properties.

#ifndef __vtkSMPropertyStatusManager_h
#define __vtkSMPropertyStatusManager_h

#include "vtkSMObject.h"
class vtkSMVectorProperty;
class vtkSMPropertyStatusManagerInternals;
class vtkSMProperty;

class VTK_EXPORT vtkSMPropertyStatusManager : public vtkSMObject
{
public:
  static vtkSMPropertyStatusManager* New();
  vtkTypeRevisionMacro(vtkSMPropertyStatusManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Register a property with the manager. A property must be registered
  // before its status can be tracked. 
  void RegisterProperty(vtkSMVectorProperty* property);
  
  // Description:
  // Unregister a property.
  void UnregisterProperty(vtkSMVectorProperty* property);

  // Description:
  // Unregister all registered properties.
  void UnregisterAllProperties();

  // Description:
  // Must be called to initialize the status of all registered properties.
  // The manager indicates changes relative to the last call to this method.
  // Note that modifed status of properties, if any, is cleared by this method.
  void InitializeStatus();

  // Description
  // Indicates if the property has modified since the last call to InitializeStatus().
  int HasPropertyChanged(vtkSMVectorProperty* property);

  // Description
  // Indicates if the element at the specified index in the property has 
  // modified since the last call to InitializeStatus().
  int HasPropertyChanged(vtkSMVectorProperty* property, int index);

  // Description:
  // Same as InitializeStatus except for a single property.
  void InitializePropertyStatus(vtkSMVectorProperty* property);

  vtkSMVectorProperty* GetInternalProperty(vtkSMVectorProperty* property);

protected:
  vtkSMPropertyStatusManager();
  ~vtkSMPropertyStatusManager();

  vtkSMPropertyStatusManagerInternals* Internals;
  vtkSMVectorProperty* DuplicateProperty(vtkSMVectorProperty* src, 
    vtkSMVectorProperty* dest = NULL);

  int HasPropertyChangedInternal(vtkSMVectorProperty* src, 
    vtkSMVectorProperty* dest, int index);
private:
  vtkSMPropertyStatusManager(const vtkSMPropertyStatusManager&); // Not implemented.
  void operator=(const vtkSMPropertyStatusManager&); // Not implemented.
};

#endif
