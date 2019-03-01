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
/**
 * @class   vtkSIProxy
 *
 * vtkSIProxy is the server-implementation for a vtkSMProxy that helps the
 * vtkSMProxy with managing/updating VTK objects.
*/

#ifndef vtkSIProxy_h
#define vtkSIProxy_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIObject.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkAlgorithmOutput;
class vtkSIProperty;
class vtkPVXMLElement;
class vtkSIProxyDefinitionManager;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIProxy : public vtkSIObject
{
public:
  static vtkSIProxy* New();
  vtkTypeMacro(vtkSIProxy, vtkSIObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method is called before the deletion of the SIObject.
   * Basically this is used to remove all dependency with other SIObject so after
   * a first pass on all SIObject with a AboutToDelete() we can simply delete the
   * remaining SIObjects.
   */
  void AboutToDelete() override;

  /**
   * Push a new state to the underneath implementation
   */
  void Push(vtkSMMessage* msg) override;

  /**
   * Pull the current state of the underneath implementation
   */
  void Pull(vtkSMMessage* msg) override;

  //@{
  /**
   * Returns access to the VTKObject pointer, if any.
   * Note this is a raw pointer to the local instance of the VTK object. Any
   * changes you make directly using this object pointer will not be reflected
   * on other processes.
   */
  vtkObjectBase* GetVTKObject();
  void SetVTKObject(vtkObjectBase*);
  //@}

  /**
   * Provides access to the property helper.
   */
  vtkSIProperty* GetSIProperty(const char* name);

  //@{
  /**
   * Returns the VTKClassName.
   */
  vtkGetStringMacro(VTKClassName);
  //@}

  //@{
  /**
   * Assigned by the XML parser. The name assigned in the XML
   * configuration. Can be used to figure out the origin of the
   * proxy.
   */
  vtkGetStringMacro(XMLName);
  //@}

  //@{
  /**
   * Assigned by the XML parser. The group in the XML configuration that
   * this proxy belongs to. Can be used to figure out the origin of the
   * proxy.
   */
  vtkGetStringMacro(XMLGroup);
  //@}

  /**
   * Return true if that Proxy is supposed to have NO vtk class, which means
   * its a NullProxy.
   */
  bool IsNullProxy() { return (VTKClassName == NULL); };

  //@{
  /**
   * These methods are called to add/remove input connections by
   * vtkSIInputProperty. This indirection makes it possible for subclasses to
   * insert VTK-algorithms in the input pipeline.
   */
  virtual void AddInput(int input_port, vtkAlgorithmOutput* connection, const char* method);
  virtual void CleanInputs(const char* method);
  //@}

  /**
   * Triggers UpdateInformation() on vtkObject if possible.
   * Default implementation does pretty much nothing.
   */
  virtual void UpdatePipelineInformation() {}

  /**
   * Called by vtkSMProxy::RecreateVTKObjects() to re-create the VTK objects.
   */
  virtual void RecreateVTKObjects();

  /**
   * Counter part to vtkSMSelfGeneratingSourceProxy::ExtendDefinition(). Called by that
   * method to let the vtkSIProxy know about the extended definition.
   */
  virtual bool ExtendDefinition(const char* xml);

  /**
   * A helper that makes up an default name if none is provided.
   */
  const char* GetLogNameOrDefault();

protected:
  vtkSIProxy();
  ~vtkSIProxy() override;

  /**
   * Create an instance of the class specified. Default implementation uses
   * `this->Interpreter->NewInstance()`.
   */
  virtual vtkObjectBase* NewVTKObject(const char* className);

  /**
   * Returns the subproxy helper for the subproxy with the given name, if any.
   */
  vtkSIProxy* GetSubSIProxy(const char* name);

  //@{
  /**
   * API to iterate over subproxy helpers.
   */
  unsigned int GetNumberOfSubSIProxys();
  vtkSIProxy* GetSubSIProxy(unsigned int cc);
  //@}

  /**
   * Provides access to the vtkSIProxyDefinitionManager held by the session.
   */
  vtkSIProxyDefinitionManager* GetProxyDefinitionManager();

  /**
   * Creates the VTK objects. This also parses  the xml definition for proxy to
   * create property-handlers.
   * Returns true if object are (or have been) created successfully.
   * \c message is used to obtain information about what proxy helper this is if
   * the objects need to be created.
   */
  virtual bool CreateVTKObjects();

  /**
   * Called to delete VTK objects.
   */
  virtual void DeleteVTKObjects();

  /**
   * Called after CreateVTKObjects(). The main difference for subclasses when
   * overriding CreateVTKObjects() or OnCreateVTKObjects() is that
   * CreateVTKObjects() is called before ReadXMLAttributes() is called, while
   * OnCreateVTKObjects() is called after ReadXMLAttributes().
   */
  virtual void OnCreateVTKObjects();

  /**
   * Called by vtkSIProxy::Push() to ensure that the vtkSIProxy has been
   * initialized.
   */
  bool InitializeAndCreateVTKObjects(vtkSMMessage* message);

  //@{
  /**
   * Parses the XML to create property/subproxy helpers.
   */
  virtual bool ReadXMLAttributes(vtkPVXMLElement* element);
  virtual bool ReadXMLProperty(vtkPVXMLElement* property_element);
  virtual bool ReadXMLSubProxy(vtkPVXMLElement* subproxy_element);
  //@}

  /**
   * Adds a vtkSMProperty's server-implementation.
   */
  void AddSIProperty(const char* name, vtkSIProperty*);

  vtkSetStringMacro(VTKClassName);
  vtkSetStringMacro(XMLGroup);
  vtkSetStringMacro(XMLName);
  vtkSetStringMacro(XMLSubProxyName);
  vtkSetStringMacro(PostPush);
  vtkSetStringMacro(PostCreation);
  vtkGetStringMacro(LogName);

  void SetLogName(const char* name);

  char* VTKClassName;
  char* XMLGroup;
  char* XMLName;
  char* XMLSubProxyName;
  char* PostPush;
  char* PostCreation;
  int NumberOfInputPorts;

  vtkSmartPointer<vtkObjectBase> VTKObject;
  bool ObjectsCreated;

private:
  vtkSIProxy(const vtkSIProxy&) = delete;
  void operator=(const vtkSIProxy&) = delete;

  class vtkInternals;
  vtkInternals* Internals;

  char* LogName;
  std::string DefaultLogName;
};

#endif
