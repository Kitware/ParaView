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
// vtkSMProperty throws a vtkCommand::Modified event everything time
// the property is modified. It does not imply that the value of the
// property actually changed. This class provides a mechanism to check
// if the value of a property actually changed. This class can only manage 
// the status of vtkSMVectorProperty subclasses. One must use
// RegisterProperty to register all the properties whose status we are 
// intersted in. Then, one can use InitializeStatus(), HasPropertyChanged()
// to reset/query the manager.

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

  // Description:
  // Internal property is the property with which the status of the property
  // is compared. Whenever a property is registered an internal property
  // similar to the property is created. The values of this internal property
  // reflect the state of the property when InitializePropertyStatus() was last
  // called.
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
