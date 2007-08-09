/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtractSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExtractSelection - Adds a second port to vtkExtractSelection, the second port contains an id selection.
// .SECTION Description
// vtkPVExtractSelection adds a second port to its vtkExtractSelection. The
// first output (from the parent class) is useful for displaying a subset of
// a dataset. The second output (the one added by this class) is a vtkSelection
// of type INDICES. This second output is useful for correlating particular 
// cells in the subset with the original data set. This is used for instance
// by ParaView's Spreadsheet view.

// .SECTION See Also
// vtkExtractSelection vtkSelection

#ifndef __vtkPVExtractSelection_h
#define __vtkPVExtractSelection_h

#include "vtkExtractSelection.h"

class VTK_EXPORT vtkPVExtractSelection : public vtkExtractSelection
{
public:
  vtkTypeRevisionMacro(vtkPVExtractSelection,vtkExtractSelection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor
  static vtkPVExtractSelection *New();

protected:
  vtkPVExtractSelection();
  ~vtkPVExtractSelection();

  //sets up empty output dataset
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);
 
  //runs the algorithm and fills the output with results
  virtual int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);

  virtual int FillOutputPortInformation(int port, vtkInformation* info);

private:
  vtkPVExtractSelection(const vtkPVExtractSelection&);  // Not implemented.
  void operator=(const vtkPVExtractSelection&);  // Not implemented.
};

#endif
