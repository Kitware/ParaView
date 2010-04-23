/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGeometryInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGeometryInformation - Info about geometry output.
// .SECTION Description
// This object collects information about the geometry filter's
// output.  It is almost the same as the superclass vtkPVDatainformation
// but the geometry filter (or update suppressor) 
// is passed in instead of the data object.

#ifndef __vtkPVGeometryInformation_h
#define __vtkPVGeometryInformation_h

#include "vtkPVDataInformation.h"

class VTK_EXPORT vtkPVGeometryInformation : public vtkPVDataInformation
{
public:
  static vtkPVGeometryInformation* New();
  vtkTypeMacro(vtkPVGeometryInformation, vtkPVDataInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

protected:
  vtkPVGeometryInformation();
  ~vtkPVGeometryInformation();

private:
  vtkPVGeometryInformation(const vtkPVGeometryInformation&); // Not implemented
  void operator=(const vtkPVGeometryInformation&); // Not implemented
};

#endif
