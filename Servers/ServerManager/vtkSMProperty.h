/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProperty - abstract superclass for all SM properties
// .SECTION Description
// vtkSMProperty is an abstract class that represents a parameter of
// a vtk object stored on one or more client manager or server nodes.
// It has a state and can push this state to the vtk object it
// refers to.

#ifndef __vtkSMProperty_h
#define __vtkSMProperty_h

#include "vtkSMObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkClientServerStream;
class vtkPVXMLElement;
class vtkSMDomain;
class vtkSMXMLParser;
//BTX
struct vtkSMPropertyInternals;
//ETX

class VTK_EXPORT vtkSMProperty : public vtkSMObject
{
public:
  static vtkSMProperty* New();
  vtkTypeRevisionMacro(vtkSMProperty, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The command name used to set the value on the server object.
  // For example: SetThetaResolution
  vtkSetStringMacro(Command);
  vtkGetStringMacro(Command);

  // Description:
  vtkSetMacro(ImmediateUpdate, int);
  vtkGetMacro(ImmediateUpdate, int);

  // Description:
  vtkSetMacro(UpdateSelf, int);
  vtkGetMacro(UpdateSelf, int);

protected:
  vtkSMProperty();
  ~vtkSMProperty();

  //BTX
  friend class vtkSMProxyManager;
  friend class vtkSMProxy;

  // Description:
  // Append a command to update the vtk object with the property values(s).
  // The proxy objects create a stream by calling this method on all the
  // modified properties.
  virtual void AppendCommandToStream(
    vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkPVXMLElement* element);

  // Description:
  void AddDomain(vtkSMDomain* dom);

  // Description:
  vtkSMDomain* GetDomain(unsigned int idx);

  // Description:
  unsigned int GetNumberOfDomains();

  char* Command;

  vtkSMPropertyInternals* PInternals;

  int ImmediateUpdate;
  int UpdateSelf;

private:
  vtkSMProperty(const vtkSMProperty&); // Not implemented
  void operator=(const vtkSMProperty&); // Not implemented
};

#endif
