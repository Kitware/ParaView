/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSubPropertyIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSubPropertyIterator - iterates over the sub-properties of a property
// .SECTION Description
// vtkSMSubPropertyIterator iterates over the sub-properties of a property

#ifndef __vtkSMSubPropertyIterator_h
#define __vtkSMSubPropertyIterator_h

#include "vtkSMObject.h"

//BTX
struct vtkSMSubPropertyIteratorInternals;
//ETX

class vtkSMSubProperty;
class vtkSMProperty;

class VTK_EXPORT vtkSMSubPropertyIterator : public vtkSMObject
{
public:
  static vtkSMSubPropertyIterator* New();
  vtkTypeMacro(vtkSMSubPropertyIterator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the property to be used.
  void SetProperty(vtkSMProperty* property);

  // Description:
  // Return the property.
  vtkGetObjectMacro(Property, vtkSMProperty);

  // Description:
  // Go to the first property.
  void Begin();

  // Description:
  // Returns true if iterator points past the end of the collection.
  int IsAtEnd();

  // Description:
  // Move to the next property.
  void Next();

  // Description:
  // Returns the key (name) at the current iterator position.
  const char* GetKey();

  // Description:
  // Returns the property at the current iterator position.
  vtkSMProperty* GetSubProperty();

protected:
  vtkSMSubPropertyIterator();
  ~vtkSMSubPropertyIterator();

  vtkSMProperty* Property;

private:
  vtkSMSubPropertyIteratorInternals* Internals;

  vtkSMSubPropertyIterator(const vtkSMSubPropertyIterator&); // Not implemented
  void operator=(const vtkSMSubPropertyIterator&); // Not implemented
};

#endif
