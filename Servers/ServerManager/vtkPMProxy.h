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
// .NAME vtkPMProxy
// .SECTION Description
// vtkPMProxy is the server-side helper for a vtkSMProxy that helps the
// vtkSMProxy with manging/updating the VTK object.

// FIXME: NEED A BETTER NAME FOR THIS CLASS
#ifndef __vtkPMProxy_h
#define __vtkPMProxy_h

#include "vtkPMObject.h"
#include "vtkClientServerID.h"

class vtkSMProxyDefinitionManager;
class vtkPVXMLElement;
class vtkPMProperty;

class VTK_EXPORT vtkPMProxy : public vtkPMObject
{
public:
  static vtkPMProxy* New();
  vtkTypeMacro(vtkPMProxy, vtkPMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Push a new state to the underneath implementation
  virtual void Push(vtkSMMessage* msg);

  // Description:
  // Pull the current state of the underneath implementation
  virtual void Pull(vtkSMMessage* msg);

  // Description:
  // Invoke a given method on the underneath objects
  virtual void Invoke(vtkSMMessage* msg);
//ETX

//BTX
  // Description:
  // Provides access to the id for the VTK object this proxy holds, if any;
  // returns vtkClientServerID(0) otherwise.
  vtkGetMacro(VTKObjectID, vtkClientServerID);

  // Description:
  // Returns access to the VTKObject pointer, if any.
  // Note this is a raw pointer to the local instance of the VTK object. Any
  // changes you make directly using this object pointer will not be reflected
  // on other processes.
  vtkObjectBase* GetVTKObject();

  // Description:
  // Provides access to the property helper.
  vtkPMProperty* GetPropertyHelper(const char* name);

protected:
  vtkPMProxy();
  ~vtkPMProxy();

  // Description:
  // Returns the subproxy helper for the subproxy with the given name, if any.
  vtkPMProxy* GetSubProxyHelper(const char* name);

  // Description:
  // Provides access to the vtkSMProxyDefinitionManager held by the session.
  vtkSMProxyDefinitionManager* GetProxyDefinitionManager();

  // Description:
  // Creates the VTK objects. This also parses  the xml definition for proxy to
  // create property-handlers.
  // Returns true if object are (or have been) created successfully.
  // \c message is used to obtain information about what proxy helper this is if
  // the objects need to be created.
  virtual bool CreateVTKObjects(vtkSMMessage* message);
  void DeleteVTKObjects();

  // Description:
  // Parses the XML to create property/subproxy helpers.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);
  virtual bool ReadXMLProperty(vtkPVXMLElement* property_element);
  virtual bool ReadXMLSubProxy(vtkPVXMLElement* subproxy_element);

  // Description:
  // Adds a property helper.
  void AddPropertyHelper(const char* name, vtkPMProperty*);

  vtkSetStringMacro(VTKClassName);
  vtkSetStringMacro(XMLGroup);
  vtkSetStringMacro(XMLName);
  vtkSetStringMacro(PostPush);
  vtkSetStringMacro(PostCreation);

  char* VTKClassName;
  char* XMLGroup;
  char* XMLName;
  char* PostPush;
  char* PostCreation;

  vtkWeakPointer<vtkObjectBase> VTKObject;
  vtkClientServerID VTKObjectID;
  bool ObjectsCreated;

private:
  vtkPMProxy(const vtkPMProxy&); // Not implemented
  void operator=(const vtkPMProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;

  // FIXME COLLABORATION : For now we dynamically convert InformationHelper
  // into the correct kernel_class and attribute sets.
  // THIS CODE MUST BE REMOVED once InformationHelper have been removed from
  // legacy XML
  void PatchXMLProperty(vtkPVXMLElement* propElement);
//ETX
};

#endif
