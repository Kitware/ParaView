/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGlobalPropertiesManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMGlobalPropertiesManager - manager for application wide properties.
// .SECTION Description
// ParaView has notion of "global properties". These are application wide
// properties such as foreground color, text color etc. Changing values of
// these properties affects all objects that are linked to these properties.
// This class provides convenient API to setup/remove such links.
//
// This is a vtkSMProxy subclass only so that the global properties can be
// defined in XML configuration. It should be treated more like a manager than
// proxy.

#ifndef __vtkSMGlobalPropertiesManager_h
#define __vtkSMGlobalPropertiesManager_h

#include "vtkSMProxy.h"
class VTK_EXPORT vtkSMGlobalPropertiesManager : public vtkSMProxy
{
public:
  static vtkSMGlobalPropertiesManager* New();
  vtkTypeMacro(vtkSMGlobalPropertiesManager, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the manager using the xml definition.
  bool InitializeProperties(const char* xmlgroup, const char* xmlname);

  // Description:
  // Returns the name of the global property with which the given property is
  // linked. NULL is returned if the property is not linked with any global
  // properties.
  const char* GetGlobalPropertyName(vtkSMProxy* proxy, const char* propname);

  // Description:
  // Sets up a link between the global property and the property if none already
  // exists. If the property is already linked with some other global property,
  // then that link is broken, since a property can be linked to only one global
  // property at a time.
  void SetGlobalPropertyLink(const char* globalPropertyName,
    vtkSMProxy*, const char* propname);

  // Description:
  // Remove a link.
  void RemoveGlobalPropertyLink(const char* globalPropertyName,
    vtkSMProxy*, const char* propname);

  // Description:
  // Saves the links state.
  virtual vtkPVXMLElement* SaveLinkState(vtkPVXMLElement* root);

  // Description:
  // Loads the state for the links.
  virtual int LoadLinkState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

//BTX
  struct ModifiedInfo
    {
    bool AddLink;
    const char* GlobalPropertyName;
    vtkSMProxy* Proxy;
    const char* PropertyName;
    };

  enum Events
    {
    GlobalPropertyLinkModified=3000,
    };

protected:
  vtkSMGlobalPropertiesManager();
  ~vtkSMGlobalPropertiesManager();

  // Description:
  // Overridden to propagate the modification to the linked properties.
  virtual void SetPropertyModifiedFlag(const char* name, int flag);

private:
  vtkSMGlobalPropertiesManager(const vtkSMGlobalPropertiesManager&); // Not implemented
  void operator=(const vtkSMGlobalPropertiesManager&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
