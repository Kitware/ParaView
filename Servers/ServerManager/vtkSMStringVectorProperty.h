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
// .NAME vtkSMStringVectorProperty - property representing a vector of strings
// .SECTION Description
// vtkSMStringVectorProperty is a concrete sub-class of vtkSMVectorProperty
// representing a vector of strings. vtkSMStringVectorProperty can also
// be used to store double and int values as strings. The strings
// are converted to the appropriate type when they are being passed
// to the stream. This is generally used for calling methods that have mixed 
// type arguments.
// .SECTION See Also
// vtkSMVectorProperty vtkSMDoubleVectorProperty vtkSMIntVectorProperty

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
  // Returns the size of the vector.
  virtual unsigned int GetNumberOfElements();

  // Description:
  // Sets the size of the vector. If num is larger than the current
  // number of elements, this may cause reallocation and copying.
  virtual void SetNumberOfElements(unsigned int num);

  // Description:
  // Set the value of 1 element. The vector is resized as necessary.
  void SetElement(unsigned int idx, const char* value);

  // Description:
  // Returns the value of 1 element.
  const char* GetElement(unsigned int idx);

  // Description:
  // Set the cast type used when passing a value to the stream.
  // For example, if the type is INT, the string is converted
  // to an int (with atoi()) before being passed to stream.
  // Note that representing scalar values as strings can result
  // in loss of accuracy.
  // Possible values are: INT, DOUBLE, STRING.
  void SetElementType(unsigned int idx, int type);

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
    vtkSMProxy*, vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  virtual int ReadXMLAttributes(vtkPVXMLElement* element);

  virtual void SaveState(const char* name, ofstream* file, vtkIndent indent);

private:
  vtkSMStringVectorProperty(const vtkSMStringVectorProperty&); // Not implemented
  void operator=(const vtkSMStringVectorProperty&); // Not implemented
};

#endif
