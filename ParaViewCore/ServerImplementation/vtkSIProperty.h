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
// .NAME vtkSIProperty
// .SECTION Description
// Basic ServerImplementation property used to call a method with no argument
// on a VTK object. If overriden, we naturally allow to call a method either
// to set or get a set of values.

#ifndef __vtkSIProperty_h
#define __vtkSIProperty_h

#include "vtkObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkClientServerStream;
class vtkSIObject;
class vtkSIProxy;
class vtkPVXMLElement;

class VTK_EXPORT vtkSIProperty : public vtkObject
{
public:
  static vtkSIProperty* New();
  vtkTypeMacro(vtkSIProperty, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The name assigned by the xml parser. Used to get the property
  // from a proxy.
  vtkGetStringMacro(XMLName);

  // Description:
  // The command name used to set the value on the server object.
  // For example: SetThetaResolution
  vtkGetStringMacro(Command);

  // Description:
  // Advanced. If UpdateSelf is true, the property will be pushed
  // by calling the method (Command) on the proxy instead of the
  // VTK object. This is commonly used to implement more complicated
  // functionality than can be obtained by calling a method on all
  // server objects.
  vtkGetMacro(UpdateSelf, bool);

  // Description:
  // Is InformationOnly is set to true, this property is used to
  // get information from server instead of setting values.
  vtkGetMacro(InformationOnly, bool);

  // Description:
  // If repeatable, a property can have 1 or more values of the same kind.
  // This ivar is configured when the xml file is read and is mainly useful
  // for information (for example from python).
  vtkGetMacro(Repeatable, bool);

  // Description:
  // This ivar is configured when the xml file is read and is mainly useful
  // to trigger a method call.
  // Internal properties are not saved in state
  vtkSetMacro(IsInternal, bool);
  vtkGetMacro(IsInternal, bool);

//BTX
protected:
  vtkSIProperty();
  ~vtkSIProperty();

  friend class vtkSIProxy;

  // Description:
  // Convenience method to obtain a vtkSIObject subclass given its global id.
  vtkSIObject* GetSIObject(vtkTypeUInt32 globalid);

  // Description:
  // Push a new state to the underneath implementation
  virtual bool Push(vtkSMMessage*, int);

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*) {return true; }

  // Description:
  // Parse the xml for the property.
  virtual bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element);

  // Description:
  // Interprets the message.
  bool ProcessMessage(vtkClientServerStream& stream);
  const vtkClientServerStream& GetLastResult();
  vtkObjectBase* GetVTKObject();

  vtkSetStringMacro(Command);
  vtkSetStringMacro(XMLName);

  char* XMLName;
  char* Command;
  bool UpdateSelf;
  bool InformationOnly;
  bool Repeatable;
  bool IsInternal;

  vtkWeakPointer<vtkSIProxy> SIProxyObject;

private:
  vtkSIProperty(const vtkSIProperty&); // Not implemented
  void operator=(const vtkSIProperty&); // Not implemented
//ETX
};

#endif
