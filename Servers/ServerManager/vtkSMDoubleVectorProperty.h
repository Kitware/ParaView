/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDoubleVectorProperty -
// .SECTION Description

#ifndef __vtkSMDoubleVectorProperty_h
#define __vtkSMDoubleVectorProperty_h

#include "vtkSMVectorProperty.h"

//BTX
struct vtkSMDoubleVectorPropertyInternals;
//ETX

class VTK_EXPORT vtkSMDoubleVectorProperty : public vtkSMVectorProperty
{
public:
  static vtkSMDoubleVectorProperty* New();
  vtkTypeRevisionMacro(vtkSMDoubleVectorProperty, vtkSMVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual int GetNumberOfElements();

  // Description:
  virtual void SetNumberOfElements(int num);

  // Description:
  void SetElement(int idx, double value);

  // Description:
  void SetElements(double* values);

  // Description:
  void SetElements1(double value0);

  // Description:
  void SetElements2(double value0, double value1);

  // Description:
  void SetElements3(double value0, double value1, double value2);

  // Description:
  double GetElement(int idx);

protected:
  vtkSMDoubleVectorProperty();
  ~vtkSMDoubleVectorProperty();

//BTX  
  // Description:
  // Update the vtk object (with the given id and on the given
  // nodes) with the property values(s).
  virtual void AppendCommandToStream(
    vtkClientServerStream* stream, vtkClientServerID objectId );
//ETX

  vtkSMDoubleVectorPropertyInternals* Internals;

private:
  vtkSMDoubleVectorProperty(const vtkSMDoubleVectorProperty&); // Not implemented
  void operator=(const vtkSMDoubleVectorProperty&); // Not implemented
};

#endif
