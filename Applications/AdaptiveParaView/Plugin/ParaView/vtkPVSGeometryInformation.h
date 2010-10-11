/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSGeometryInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSGeometryInformation - Info about geometry output.
// .SECTION Description
// This object collects information about the geometry filter's
// output.  It is almost the same as the superclass vtkPVDatainformation
// but it uses meta information when available to get the whole picture
// without requiring a full pipeline update.

#ifndef __vtkPVSGeometryInformation_h
#define __vtkPVSGeometryInformation_h

#include "vtkPVGeometryInformation.h"

class VTK_EXPORT vtkPVSGeometryInformation : public vtkPVGeometryInformation
{
public:
  static vtkPVSGeometryInformation* New();
  vtkTypeMacro(vtkPVSGeometryInformation, vtkPVGeometryInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromDataSet(vtkDataSet*);

protected:
  vtkPVSGeometryInformation();
  ~vtkPVSGeometryInformation();

private:
  vtkPVSGeometryInformation(const vtkPVSGeometryInformation&);// Not implemented
  void operator=(const vtkPVSGeometryInformation&); // Not implemented
};

#endif
