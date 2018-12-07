/*=========================================================================

  Program:   ParaView
  Module:    vtkSIProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIProperty
 *
 * Basic ServerImplementation property used to call a method with no argument
 * on a VTK object. If overridden, we naturally allow to call a method either
 * to set or get a set of values.
*/

#ifndef vtkSIProperty_h
#define vtkSIProperty_h

#include "vtkObject.h"
#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"                 // needed for vtkSMMessage
#include "vtkWeakPointer.h"                      // needed for vtkWeakPointer.

class vtkClientServerStream;
class vtkSIObject;
class vtkSIProxy;
class vtkPVXMLElement;

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSIProperty : public vtkObject
{
public:
  static vtkSIProperty* New();
  vtkTypeMacro(vtkSIProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The name assigned by the xml parser. Used to get the property
   * from a proxy.
   */
  vtkGetStringMacro(XMLName);
  //@}

  //@{
  /**
   * The command name used to set the value on the server object.
   * For example: SetThetaResolution
   */
  vtkGetStringMacro(Command);
  //@}

  //@{
  /**
   * Is InformationOnly is set to true, this property is used to
   * get information from server instead of setting values.
   */
  vtkGetMacro(InformationOnly, bool);
  //@}

  //@{
  /**
   * If repeatable, a property can have 1 or more values of the same kind.
   * This ivar is configured when the xml file is read and is mainly useful
   * for information (for example from python).
   */
  vtkGetMacro(Repeatable, bool);
  //@}

  //@{
  /**
   * This ivar is configured when the xml file is read and is mainly useful
   * to trigger a method call.
   * Internal properties are not saved in state
   */
  vtkSetMacro(IsInternal, bool);
  vtkGetMacro(IsInternal, bool);
  //@}

protected:
  vtkSIProperty();
  ~vtkSIProperty() override;

  friend class vtkSIProxy;

  /**
   * Convenience method to obtain a vtkSIObject subclass given its global id.
   */
  vtkSIObject* GetSIObject(vtkTypeUInt32 globalid);

  /**
   * Push a new state to the underneath implementation
   */
  virtual bool Push(vtkSMMessage*, int);

  /**
   * Pull the current state of the underneath implementation
   */
  virtual bool Pull(vtkSMMessage*);

  /**
   * Parse the xml for the property.
   */
  virtual bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element);

  //@{
  /**
   * Interprets the message.
   */
  bool ProcessMessage(vtkClientServerStream& stream);
  const vtkClientServerStream& GetLastResult();
  vtkObjectBase* GetVTKObject();
  //@}

  vtkSetStringMacro(Command);
  vtkSetStringMacro(XMLName);

  char* XMLName;
  char* Command;
  bool InformationOnly;
  bool Repeatable;
  bool IsInternal;

  vtkWeakPointer<vtkSIProxy> SIProxyObject;

  // Allow subclass to save the property value as a state part
  void SaveValueToCache(vtkSMMessage* proxyStateUpdate, int offset);

private:
  vtkSIProperty(const vtkSIProperty&) = delete;
  void operator=(const vtkSIProperty&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
