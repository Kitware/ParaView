/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIntVectorProperty -
// .SECTION Description

#ifndef __vtkSMIntVectorProperty_h
#define __vtkSMIntVectorProperty_h

#include "vtkSMVectorProperty.h"

//BTX
struct vtkSMIntVectorPropertyInternals;
//ETX

class VTK_EXPORT vtkSMIntVectorProperty : public vtkSMVectorProperty
{
public:
  static vtkSMIntVectorProperty* New();
  vtkTypeRevisionMacro(vtkSMIntVectorProperty, vtkSMVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual int GetNumberOfElements();

  // Description:
  virtual void SetNumberOfElements(int num);

  // Description:
  void SetElement(int idx, int value);

  // Description:
  void SetElements(int* values);

  // Description:
  void SetElements1(int value0);

  // Description:
  void SetElements2(int value0, int value1);

  // Description:
  void SetElements3(int value0, int value1, int value2);

  // Description:
  int GetElement(int idx);

protected:
  vtkSMIntVectorProperty();
  ~vtkSMIntVectorProperty();

  vtkSMIntVectorPropertyInternals* Internals;

  //BTX  
  // Description:
  // Update the vtk object (with the given id and on the given
  // nodes) with the property values(s).
  virtual void AppendCommandToStream(
    vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

private:
  vtkSMIntVectorProperty(const vtkSMIntVectorProperty&); // Not implemented
  void operator=(const vtkSMIntVectorProperty&); // Not implemented
};

#endif
