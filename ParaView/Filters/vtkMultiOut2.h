/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiOut2.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiOut2 - Test a source with multiple outputs.
// .SECTION Description
// A test for a group of parts in one module.

#ifndef __vtkMultiOut2_h
#define __vtkMultiOut2_h

#include "vtkDataSetSource.h"

class VTK_EXPORT vtkMultiOut2 : public vtkDataSetSource
{
public:
  static vtkMultiOut2* New();
  vtkTypeRevisionMacro(vtkMultiOut2,vtkDataSetSource);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMultiOut2();
  ~vtkMultiOut2();

  void Execute();

private:
  vtkMultiOut2(const vtkMultiOut2&);  // Not implemented.
  void operator=(const vtkMultiOut2&);  // Not implemented.
};

#endif

