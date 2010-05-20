/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDomainIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDomainIterator - iterates over domains of a property
// .SECTION Description
// vtkSMDomainIterator iterates over the domains of a property.

#ifndef __vtkSMDomainIterator_h
#define __vtkSMDomainIterator_h

#include "vtkSMObject.h"

class vtkSMProperty;
class vtkSMDomain;

//BTX
struct vtkSMDomainIteratorInternals;
//ETX

class VTK_EXPORT vtkSMDomainIterator : public vtkSMObject
{
public:
  static vtkSMDomainIterator* New();
  vtkTypeMacro(vtkSMDomainIterator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // A property must be assigned before iteration is performed.
  void SetProperty(vtkSMProperty* property);

  // Description:
  // Returns the property being iterated over.
  vtkGetObjectMacro(Property, vtkSMProperty);

  // Description:
  // Go to the first domain.
  void Begin();

  // Description:
  // Is the iterator at the end of the list.
  int IsAtEnd();

  // Description:
  // Move to the next iterator.
  void Next();

  // Description:
  // Returns the key (the name) of the current domain.
  const char* GetKey();

  // Description:
  // Returns the current domain.
  vtkSMDomain* GetDomain();

protected:
  vtkSMDomainIterator();
  ~vtkSMDomainIterator();

  vtkSMProperty* Property;

private:
  vtkSMDomainIteratorInternals* Internals;

  vtkSMDomainIterator(const vtkSMDomainIterator&); // Not implemented
  void operator=(const vtkSMDomainIterator&); // Not implemented
};

#endif
