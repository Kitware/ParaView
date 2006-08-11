/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockExtractSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiBlockExtractSelection - extract cells given a selection
// .SECTION Description
// vtkMultiBlockExtractSelection extracts cells using a selection. It
// traverses the selection tree, finds the data to extract from using
// SOURCE_ID (SOURCE_ID being the client/server id) and extract the
// cells in the selection list.

#ifndef __vtkMultiBlockExtractSelection_h
#define __vtkMultiBlockExtractSelection_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkDataSet;
class vtkExtractSelection;
class vtkPolyDataExtractSelection;
class vtkSelection;

class VTK_EXPORT vtkMultiBlockExtractSelection : public vtkMultiBlockDataSetAlgorithm 
{
public:
  static vtkMultiBlockExtractSelection *New();
  vtkTypeRevisionMacro(vtkMultiBlockExtractSelection,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the MTime taking into account changes to the selection
  unsigned long GetMTime();

  // Description:
  // Specify the implicit function for inside/outside checks. The selection
  // must have a CONTENT_TYPE of CELL_IDS and have a vtkIdTypeArray
  // containing the cell id list.
  virtual void SetSelection(vtkSelection*);
  vtkGetObjectMacro(Selection,vtkSelection);

protected:
  vtkMultiBlockExtractSelection();
  ~vtkMultiBlockExtractSelection();

  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);
  virtual int RequestInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*);

  vtkSelection *Selection;
  
private:
  vtkDataSet* SelectFromDataSet(vtkSelection* sel);
  vtkExtractSelection* ExtractFilter;

  vtkMultiBlockExtractSelection(const vtkMultiBlockExtractSelection&);  // Not implemented.
  void operator=(const vtkMultiBlockExtractSelection&);  // Not implemented.
};

#endif


