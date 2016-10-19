/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositePolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMantaCompositeMapper - MantaMapper for composite data
// .SECTION Description
// This class is an adapter between composite data produced by the data
// processing pipeline and the non composite capable vtkMantaPolyDataMapper.

#ifndef vtkMantaCompositeMapper_h
#define vtkMantaCompositeMapper_h

#include "vtkCompositePolyDataMapper.h"
#include "vtkMantaModule.h"
class vtkPolyDataMapper;

class VTKMANTA_EXPORT vtkMantaCompositeMapper : public vtkCompositePolyDataMapper
{

public:
  static vtkMantaCompositeMapper* New();
  vtkTypeMacro(vtkMantaCompositeMapper, vtkCompositePolyDataMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Helper to cleanly track our inputs mtime, and participate in manta
  // state tracking.
  virtual unsigned long GetInputTime();

protected:
  vtkMantaCompositeMapper();
  ~vtkMantaCompositeMapper();

  // Description:
  // Need to define the type of data handled by this mapper.
  virtual vtkPolyDataMapper* MakeAMapper();

private:
  vtkMantaCompositeMapper(const vtkMantaCompositeMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMantaCompositeMapper&) VTK_DELETE_FUNCTION;
};

#endif
