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
// .NAME vtkSMDomainIterator -
// .SECTION Description

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
  vtkTypeRevisionMacro(vtkSMDomainIterator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  void SetProperty(vtkSMProperty* property);

  // Description:
  vtkGetObjectMacro(Property, vtkSMProperty);

  // Description:
  void Begin();

  // Description:
  int IsAtEnd();

  // Description:
  void Next();

  // Description:
  const char* GetKey();

  // Description:
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
