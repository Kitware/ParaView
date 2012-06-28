/*=========================================================================

  Program:   ParaView
  Module:    vtkPVNullSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVNullSource - source for NULL data.
// .SECTION Description
// This is a source for null data. Although this actually produces a
// vtkPolyLine paraview blocks all data information from this source resulting
// in it being treated as a null source.

#ifndef __vtkPVNullSource_h
#define __vtkPVNullSource_h

#include "vtkLineSource.h"

class VTK_EXPORT vtkPVNullSource : public vtkLineSource
{
public:
  static vtkPVNullSource* New();
  vtkTypeMacro(vtkPVNullSource, vtkLineSource);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVNullSource();
  ~vtkPVNullSource();

private:
  vtkPVNullSource(const vtkPVNullSource&); // Not implemented
  void operator=(const vtkPVNullSource&); // Not implemented
//ETX
};

#endif

