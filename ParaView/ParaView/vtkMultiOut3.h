/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiOut3.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiOut3 - Test a source with multiple outputs.
// .SECTION Description
// A test for a group of parts in one module.

#ifndef __vtkMultiOut3_h
#define __vtkMultiOut3_h

#include "vtkDataSetSource.h"

class VTK_EXPORT vtkMultiOut3 : public vtkDataSetSource
{
public:
  static vtkMultiOut3* New();
  vtkTypeRevisionMacro(vtkMultiOut3,vtkDataSetSource);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMultiOut3();
  ~vtkMultiOut3();

  void Execute();
  void ExecuteInformation();

private:
  vtkMultiOut3(const vtkMultiOut3&);  // Not implemented.
  void operator=(const vtkMultiOut3&);  // Not implemented.
};

#endif

