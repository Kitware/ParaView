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
// .NAME vtkPVUpdateSuppressor - prevents propagation of update
// .SECTION Description
//
// .SECTION Caveats

// .SECTION See Also

#ifndef __vtkPVUpdateSuppressor_h
#define __vtkPVUpdateSuppressor_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_EXPORT vtkPVUpdateSuppressor : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkPVUpdateSuppressor,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function.
  static vtkPVUpdateSuppressor *New();

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
  vtkPVUpdateSuppressor();
  ~vtkPVUpdateSuppressor();

  void Execute();

private:
  vtkPVUpdateSuppressor(const vtkPVUpdateSuppressor&);  // Not implemented.
  void operator=(const vtkPVUpdateSuppressor&);  // Not implemented.
};

#endif
