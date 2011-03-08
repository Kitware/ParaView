/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRepresentedDataInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRepresentedDataInformation
// .SECTION Description
// vtkPVRepresentedDataInformation is a vtkPVDataInformation subclass that knows
// how to gather rendered data-information from a vtkPVDataRepresentation.

#ifndef __vtkPVRepresentedDataInformation_h
#define __vtkPVRepresentedDataInformation_h

#include "vtkPVDataInformation.h"

class VTK_EXPORT vtkPVRepresentedDataInformation : public vtkPVDataInformation
{
public:
  static vtkPVRepresentedDataInformation* New();
  vtkTypeMacro(vtkPVRepresentedDataInformation, vtkPVDataInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

//BTX
protected:
  vtkPVRepresentedDataInformation();
  ~vtkPVRepresentedDataInformation();

private:
  vtkPVRepresentedDataInformation(const vtkPVRepresentedDataInformation&); // Not implemented
  void operator=(const vtkPVRepresentedDataInformation&); // Not implemented
//ETX
};

#endif
