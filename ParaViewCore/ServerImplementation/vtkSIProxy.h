/*=========================================================================

  Program:   ParaView
  Module:    vtkSIProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIProxy
// .SECTION Description
// vtkSIProxy is the server-implementation for a vtkSMProxy that helps the
// vtkSMProxy with managing/updating VTK objects.

#ifndef __vtkSIProxy_h
#define __vtkSIProxy_h

#include "vtkSIObject.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkAlgorithmOutput;
class vtkSIProperty;
class vtkPVXMLElement;
class vtkSIProxyDefinitionManager;

class VTK_EXPORT vtkSIProxy : public vtkSIObject
{
public:
  static vtkSIProxy* New();
  vtkTypeMacro(vtkSIProxy, vtkSIObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Push a new state to the underneath implementation
  virtual void Push(vtkSMMessage* msg);

  // Description:
  // Pull the current state of the underneath implementation
  virtual void Pull(vtkSMMessage* msg);
//ETX

  // Description:
  // Returns access to the VTKObject pointer, if any.
  // Note this is a raw pointer to the local instance of the VTK object. Any
  // changes you make directly using this object pointer will not be reflected
  // on other processes.
  vtkObjectBase* GetVTKObject();
  void SetVTKObject(vtkObjectBase*);

  // Description:
  // Provides access to the property helper.
  vtkSIProperty* GetSIProperty(const char* name);

  // Description:
  // Returns the VTKClassName.
  vtkGetStringMacro(VTKClassName);

  // Description:
  // Return true if that Proxy is supposed to have NO vtk class, which means
  // its a NullProxy.
  bool IsNullProxy() { return (VTKClassName == NULL); };

  // Description:
  // These methods are called to add/remove input connections by
  // vtkSIInputProperty. This indirection makes it possible for subclasses to
  // insert VTK-algorithms in the input pipeline.
  virtual void AddInput(int input_port,
    vtkAlgorithmOutput* connection, const char* method);
  virtual void CleanInputs(const char* method);

  // Description:
  // Triggers UpdateInformation() on vtkObject if possible.
  // Default implementation does pretty much nothing.
  virtual void UpdatePipelineInformation() { }

//BTX
protected:
  vtkSIProxy();
  ~vtkSIProxy();

  // Description:
  // Returns the subproxy helper for the subproxy with the given name, if any.
  vtkSIProxy* GetSubSIProxy(const char* name);

  // Description:
  // API to iterate over subproxy helpers.
  unsigned int GetNumberOfSubSIProxys();
  vtkSIProxy* GetSubSIProxy(unsigned int cc);

  // Description:
  // Provides access to the vtkSIProxyDefinitionManager held by the session.
  vtkSIProxyDefinitionManager* GetProxyDefinitionManager();

  // Description:
  // Creates the VTK objects. This also parses  the xml definition for proxy to
  // create property-handlers.
  // Returns true if object are (or have been) created successfully.
  // \c message is used to obtain information about what proxy helper this is if
  // the objects need to be created.
  virtual bool CreateVTKObjects(vtkSMMessage* message);
  void DeleteVTKObjects();

  // Description;
  // Called in CreateVTKObjects() after the vtk-object has been created and
  // subproxy-information has been processed, but before the XML is parsed to
  // generate properties and initialize their values.
  virtual void OnCreateVTKObjects();

  // Description:
  // Parses the XML to create property/subproxy helpers.
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);
  virtual bool ReadXMLProperty(vtkPVXMLElement* property_element);
  virtual bool ReadXMLSubProxy(vtkPVXMLElement* subproxy_element);

  // Description:
  // Adds a vtkSMProperty's server-implementation.
  void AddSIProperty(const char* name, vtkSIProperty*);

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

  vtkSmartPointer<vtkObjectBase> VTKObject;
  bool ObjectsCreated;

private:
  vtkSIProxy(const vtkSIProxy&); // Not implemented
  void operator=(const vtkSIProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
