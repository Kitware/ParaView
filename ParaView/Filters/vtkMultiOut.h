/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiOut.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiOut - Test a source with multiple outputs.
// .SECTION Description
// A test for a group of parts in one module.

#ifndef __vtkMultiOut_h
#define __vtkMultiOut_h

#include "vtkPolyDataSource.h"

class VTK_EXPORT vtkMultiOut : public vtkPolyDataSource
{
public:
  static vtkMultiOut* New();
  vtkTypeRevisionMacro(vtkMultiOut,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMultiOut();
  ~vtkMultiOut();

  void Execute();
  int NumberOfSpheres;

private:

  vtkMultiOut(const vtkMultiOut&);  // Not implemented.
  void operator=(const vtkMultiOut&);  // Not implemented.
};

#endif

