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
// .NAME vtkSMProperty - abstract superclass for all ParaView properties
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
class vtkSMXMLParser;

class VTK_EXPORT vtkSMProperty : public vtkSMObject
{
public:
  vtkTypeRevisionMacro(vtkSMProperty, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  vtkSetStringMacro(Command);
  vtkGetStringMacro(Command);

protected:
  vtkSMProperty();
  ~vtkSMProperty();

  //BTX
  friend class vtkSMProxyManager;
  friend class vtkSMProxy;

  // Description:
  // Update the vtk object (with the given id and on the given
  // nodes) with the property values(s).
  virtual void AppendCommandToStream(
    vtkClientServerStream* stream, vtkClientServerID objectId ) = 0;
  //ETX

  virtual int ReadXMLAttributes(vtkPVXMLElement* element);

  char* Command;

private:
  vtkSMProperty(const vtkSMProperty&); // Not implemented
  void operator=(const vtkSMProperty&); // Not implemented
};

#endif
