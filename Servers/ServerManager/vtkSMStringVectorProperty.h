/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStringVectorProperty -
// .SECTION Description

#ifndef __vtkSMStringVectorProperty_h
#define __vtkSMStringVectorProperty_h

#include "vtkSMVectorProperty.h"

//BTX
struct vtkSMStringVectorPropertyInternals;
//ETX

class VTK_EXPORT vtkSMStringVectorProperty : public vtkSMVectorProperty
{
public:
  static vtkSMStringVectorProperty* New();
  vtkTypeRevisionMacro(vtkSMStringVectorProperty, vtkSMVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual int GetNumberOfElements();

  // Description:
  virtual void SetNumberOfElements(int num);

  // Description:
  void SetElement(int idx, const char* value);

  // Description:
  const char* GetElement(int idx);

  //BTX
  enum ElementTypes{ INT, DOUBLE, STRING };
  //ETX

protected:
  vtkSMStringVectorProperty();
  ~vtkSMStringVectorProperty();

  vtkSMStringVectorPropertyInternals* Internals;

  int GetElementType(unsigned int idx);

  //BTX  
  // Description:
  // Update the vtk object (with the given id and on the given
  // nodes) with the property values(s).
  virtual void AppendCommandToStream(
    vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  virtual int ReadXMLAttributes(vtkPVXMLElement* element);

private:
  vtkSMStringVectorProperty(const vtkSMStringVectorProperty&); // Not implemented
  void operator=(const vtkSMStringVectorProperty&); // Not implemented
};

#endif
