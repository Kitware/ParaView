/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNamedPropertyIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMNamedPropertyIterator - iterates over a subset of a proxy's properties
//
// .SECTION Description
// vtkSMNamedPropertyIterator can be used to iterate over a subset of a proxy's 
// properties. The subset is defined through a list of strings naming properties.
// The properties of the root proxies as well as sub-proxies are  included in the
// iteration. For sub-proxies, only exposed properties are iterated over.
//
// .SECTION See Also
// vtkSMPropertyIterator
//
// .SECTION Thanks
// This class was contributed by SciberQuest Inc.
#ifndef __vtkSMNamedPropertyIterator_h
#define __vtkSMNamedPropertyIterator_h

#include "vtkSMPropertyIterator.h"

class vtkSMProperty;
class vtkSMProxy;
class vtkStringList;

class VTK_EXPORT vtkSMNamedPropertyIterator : public vtkSMPropertyIterator
{
public:
  static vtkSMNamedPropertyIterator* New();
  vtkTypeMacro(vtkSMNamedPropertyIterator, vtkSMPropertyIterator);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the names of properties to iterate over.
  void SetPropertyNames(vtkStringList *names);

  // Description:
  // Go to the first property.
  virtual void Begin();

  // Description:
  // Returns true if iterator points past the end of the collection.
  virtual int IsAtEnd();

  // Description:
  // Move to the next property.
  virtual void Next();

  // Description:
  // Returns the key (name) at the current iterator position.
  virtual const char* GetKey();

  // Description:
  // Returns the XMLLabel for self properties and the exposed name for
  // sub-proxy properties.
  virtual const char* GetPropertyLabel();

  // Description:
  // Returns the property at the current iterator position.
  virtual vtkSMProperty* GetProperty();

protected:
  vtkSMNamedPropertyIterator();
  ~vtkSMNamedPropertyIterator();

  vtkStringList *PropertyNames;
  int PropertyNameIndex;

private:
  vtkSMNamedPropertyIterator(const vtkSMNamedPropertyIterator&); // Not implemented
  void operator=(const vtkSMNamedPropertyIterator&); // Not implemented
};

#endif

