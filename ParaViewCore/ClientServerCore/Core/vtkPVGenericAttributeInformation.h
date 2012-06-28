/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGenericAttributeInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGenericAttributeInformation - Generic attribute information like type.
// .SECTION Description
// This objects is for eliminating direct access to vtkDataObjects
// by the "client".  Only vtkPVPart and vtkPVProcessModule should access
// the data directly.  At the moment, this object is only a container
// and has no useful methods for operating on data.

#ifndef __vtkPVGenericAttributeInformation_h
#define __vtkPVGenericAttributeInformation_h

#include "vtkPVArrayInformation.h"

class vtkClientServerStream;

class VTK_EXPORT vtkPVGenericAttributeInformation : public vtkPVArrayInformation
{
public:
  static vtkPVGenericAttributeInformation* New();
  vtkTypeMacro(vtkPVGenericAttributeInformation, vtkPVArrayInformation);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

protected:
  vtkPVGenericAttributeInformation();
  ~vtkPVGenericAttributeInformation();

  vtkPVGenericAttributeInformation(const vtkPVGenericAttributeInformation&); // Not implemented
  void operator=(const vtkPVGenericAttributeInformation&); // Not implemented
};

#endif
