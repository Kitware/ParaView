/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVUpdateSupressor.h
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
// .NAME vtkPVUpdateSupressor - prevents propagation of update
// .SECTION Description
//
// .SECTION Caveats

// .SECTION See Also

#ifndef __vtkPVUpdateSupressor_h
#define __vtkPVUpdateSupressor_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkImplicitFunction.h"

class VTK_EXPORT vtkPVUpdateSupressor : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkPVUpdateSupressor,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function.
  static vtkPVUpdateSupressor *New();

  // Description:
  // Return the mtime also considering the locator and clip function.
  unsigned long GetMTime();

  // Description:
  // Force update on the input.
  void ForceUpdate();

  // Description:
  // Overwrite UpdateData so that it will not propagate update.
  void UpdateData(vtkDataObject *output);

  // Description:
  // Set number of pieces and piece on the data.
  virtual void SetUpdateNumberOfPieces(int);
  virtual void SetUpdatePiece(int);

protected:
  vtkPVUpdateSupressor();
  ~vtkPVUpdateSupressor();

  void Execute();

private:
  vtkPVUpdateSupressor(const vtkPVUpdateSupressor&);  // Not implemented.
  void operator=(const vtkPVUpdateSupressor&);  // Not implemented.
};

#endif
