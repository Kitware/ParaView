/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWarpVector.h
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
// .NAME vtkPVWarpVector - 
// .SECTION Description

#ifndef __vtkPVWarpVector_h
#define __vtkPVWarpVector_h

#include "vtkWarpVector.h"

class VTK_EXPORT vtkPVWarpVector : public vtkWarpVector
{
public:
  static vtkPVWarpVector *New();
  vtkTypeRevisionMacro(vtkPVWarpVector,vtkWarpVector);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If you want to warp by an arbitrary vector array, then set its name here.
  // By default this in NULL and the filter will use the active vector array.
  vtkGetStringMacro(InputVectorsSelection);
  void SelectInputVectors(const char *fieldName) 
    {this->SetInputVectorsSelection(fieldName);}
  
protected:
  vtkPVWarpVector();
  ~vtkPVWarpVector();

  vtkPVWarpVector(const vtkPVWarpVector&);  // Not implemented.
  void operator=(const vtkPVWarpVector&);  // Not implemented.
};

#endif
