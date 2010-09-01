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
// .NAME vtkPMProperty
// .SECTION Description
//

#ifndef __vtkPMProperty_h
#define __vtkPMProperty_h

#include "vtkClientServerID.h"
#include "vtkSMMessage.h"
#include "vtkSMObject.h"
#include "vtkWeakPointer.h"

class vtkPVXMLElement;
class vtkPMProxy;
class vtkClientServerStream;


class VTK_EXPORT vtkPMProperty : public vtkSMObject
{
public:
  static vtkPMProperty* New();
  vtkTypeMacro(vtkPMProperty, vtkSMObject);
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

//BTX
protected:
  vtkPMProperty();
  ~vtkPMProperty();

  friend class vtkPMProxy;

  vtkClientServerID GetVTKObjectID();

  // Description:
  // Push a new state to the underneath implementation
  virtual bool Push(vtkSMMessage*, int) {return true;}

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*) {return true; }

  // Description:
  // Parse the xml for the property.
  virtual bool ReadXMLAttributes(vtkPMProxy* proxyhelper, vtkPVXMLElement* element);

  // Description:
  // Interprets the message.
  bool ProcessMessage(vtkClientServerStream& stream);
  const vtkClientServerStream& GetLastResult();

  vtkSetStringMacro(Command);
  vtkSetStringMacro(XMLName);

  char* XMLName;
  char* Command;
  bool UpdateSelf;
  bool InformationOnly;
  bool Repeatable;

  vtkWeakPointer<vtkPMProxy> ProxyHelper;

private:
  vtkPMProperty(const vtkPMProperty&); // Not implemented
  void operator=(const vtkPMProperty&); // Not implemented
//ETX
};

#endif
