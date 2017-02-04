/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtractArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVExtractArraysOverTime
 * @brief   extract point or cell data over time (parallel)
 *
 * vtkPVExtractArraysOverTime is a subclass of vtkPExtractArraysOverTime
 * that overrides the default SelectionExtractor with a vtkPVExtractSelection
 * instance.
 * This enables query selections to be extracted at each time step.
 * @sa
 * vtkExtractArraysOverTime
 * vtkPExtractArraysOverTime
*/

#ifndef vtkPVExtractArraysOverTime_h
#define vtkPVExtractArraysOverTime_h

#include "vtkPExtractArraysOverTime.h"
#include "vtkPVClientServerCoreCoreModule.h" // For export macro

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVExtractArraysOverTime : public vtkPExtractArraysOverTime
{
public:
  static vtkPVExtractArraysOverTime* New();
  vtkTypeMacro(vtkPVExtractArraysOverTime, vtkPExtractArraysOverTime);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkPVExtractArraysOverTime();
  ~vtkPVExtractArraysOverTime();

private:
  vtkPVExtractArraysOverTime(const vtkPVExtractArraysOverTime&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVExtractArraysOverTime&) VTK_DELETE_FUNCTION;
};

#endif // vtkPVExtractArraysOverTime_h
