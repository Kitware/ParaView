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
// .NAME vtkSMIntVectorProperty - property representing a vector of integers
// .SECTION Description
// vtkSMIntVectorProperty is a concrete sub-class of vtkSMVectorProperty
// representing a vector of integers.
// .SECTION See Also
// vtkSMVectorProperty vtkSMDoubleVectorProperty vtkSMStringVectorProperty

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
  // Returns the size of the vector.
  virtual unsigned int GetNumberOfElements();

  // Description:
  // Sets the size of the vector. If num is larger than the current
  // number of elements, this may cause reallocation and copying.
  virtual void SetNumberOfElements(unsigned int num);

  // Description:
  // Set the value of 1 element. The vector is resized as necessary.
  void SetElement(unsigned int idx, int value);

  // Description:
  // Set the values of all elements. The size of the values array
  // has to be equal or larger to the size of the vector.
  void SetElements(const int* values);

  // Description:
  // Set the value of 1st element. The vector is resized as necessary.
  void SetElements1(int value0);

  // Description:
  // Set the values of the first 2 elements. The vector is resized as necessary.
  void SetElements2(int value0, int value1);

  // Description:
  // Set the values of the first 3 elements. The vector is resized as necessary.
  void SetElements3(int value0, int value1, int value2);

  // Description:
  // Returns the value of 1 element.
  int GetElement(unsigned int idx);

  // Description:
  // If ArgumentIsArray is true, multiple elements are passed in as
  // array arguments. For example, For example, if
  // RepeatCommand is true, NumberOfElementsPerCommand is 2, the
  // command is SetFoo and the values are 1 2 3 4 5 6, the resulting
  // stream will have:
  // @verbatim
  // * Invoke obj SetFoo array(1, 2)
  // * Invoke obj SetFoo array(3, 4)
  // * Invoke obj SetFoo array(5, 6)
  // @endverbatim
  vtkGetMacro(ArgumentIsArray, int);
  vtkSetMacro(ArgumentIsArray, int);
  vtkBooleanMacro(ArgumentIsArray, int);

protected:
  vtkSMIntVectorProperty();
  ~vtkSMIntVectorProperty();

  virtual int ReadXMLAttributes(vtkPVXMLElement* element);

  vtkSMIntVectorPropertyInternals* Internals;

  int ArgumentIsArray;

  //BTX  
  // Description:
  // Append a command to update the vtk object with the property values(s).
  // The proxy objects create a stream by calling this method on all the
  // modified properties.
  virtual void AppendCommandToStream(
    vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

private:
  vtkSMIntVectorProperty(const vtkSMIntVectorProperty&); // Not implemented
  void operator=(const vtkSMIntVectorProperty&); // Not implemented
};

#endif
