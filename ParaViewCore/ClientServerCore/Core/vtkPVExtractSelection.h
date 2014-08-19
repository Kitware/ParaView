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
// .NAME vtkPVExtractSelection - Adds a two more output ports to vtkExtractSelection,
// the second port contains an id selection and the third is simply the input
// selection.
// .SECTION Description
// vtkPVExtractSelection adds a second port to vtkExtractSelection.
// \li Output port 0 -- is the output from the superclass. It's nothing but the
// extracted dataset.
//
// \li Output port 1 -- is a vtkSelection consisting of indices of the cells/points
// extracted. If vtkSelection used as the input to this filter is of the field
// type vtkSelection::CELL, then the output vtkSelection has both the cell
// indicides as well as point indices of the cells/points that were extracted.
// If input field type is vtkSelection::POINT, then the output vtkSelection only
// has the indicies of the points that were extracted.
// This second output is useful for correlating particular
// cells in the subset with the original data set. This is used, for instance,
// by Chart representations to show selections.
//
// \li Output port 2 -- is simply the input vtkSelection. We currently use this
// for Histogram View/Representation. Since that view cannot show arbitrary ID
// based selections, it needs to get to the original vtkSelection to determine
// if the particular selection can be shown in the view at all.
// .SECTION See Also
// vtkExtractSelection vtkSelection

#ifndef __vtkPVExtractSelection_h
#define __vtkPVExtractSelection_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkExtractSelection.h"

class vtkSelectionNode;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVExtractSelection : public vtkExtractSelection
{
public:
  vtkTypeMacro(vtkPVExtractSelection,vtkExtractSelection);
  void PrintSelf(ostream& os, vtkIndent indent);

  static const int OUTPUT_PORT_EXTRACTED_DATASET = 0;
  static const int OUTPUT_PORT_SELECTION_IDS = 1;
  static const int OUTPUT_PORT_SELECTION_ORIGINAL = 2;

  // Description:
  // Constructor
  static vtkPVExtractSelection *New();

  // Description:
  // Removes all inputs from input port 1.
  void RemoveAllSelectionsInputs()
    { this->SetInputConnection(1, 0); }

//BTX
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

  vtkSelectionNode* LocateSelection(unsigned int level,
    unsigned int index, vtkSelection* sel);
  vtkSelectionNode* LocateSelection(unsigned int composite_index, vtkSelection* sel);

private:
  vtkPVExtractSelection(const vtkPVExtractSelection&);  // Not implemented.
  void operator=(const vtkPVExtractSelection&);  // Not implemented.

  class vtkSelectionNodeVector;
  void RequestDataInternal(vtkSelectionNodeVector& outputs,
                           vtkDataObject* dataObjectOutput,
                           vtkSelectionNode* sel);

  // Returns the combined content type for the selection.
  int GetContentType(vtkSelection* sel);
//ETX
};

#endif
