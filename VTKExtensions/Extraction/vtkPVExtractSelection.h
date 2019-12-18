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
/**
 * @class   vtkPVExtractSelection
 * @brief   Adds a two more output ports to vtkExtractSelection,
 * the second port contains an id selection and the third is simply the input
 * selection.
 *
 * vtkPVExtractSelection adds a second port to vtkExtractSelection.
 * \li Output port 0 -- is the output from the superclass. It's nothing but the
 * extracted dataset.
 *
 * \li Output port 1 -- is a vtkSelection consisting of indices of the cells/points
 * extracted. If vtkSelection used as the input to this filter is of the field
 * type vtkSelection::CELL, then the output vtkSelection has both the cell
 * indicides as well as point indices of the cells/points that were extracted.
 * If input field type is vtkSelection::POINT, then the output vtkSelection only
 * has the indices of the points that were extracted.
 * This second output is useful for correlating particular
 * cells in the subset with the original data set. This is used, for instance,
 * by Chart representations to show selections.
 *
 * \li Output port 2 -- is simply the input vtkSelection. We currently use this
 * for Histogram View/Representation. Since that view cannot show arbitrary ID
 * based selections, it needs to get to the original vtkSelection to determine
 * if the particular selection can be shown in the view at all.
 * @sa
 * vtkExtractSelection vtkSelection
*/

#ifndef vtkPVExtractSelection_h
#define vtkPVExtractSelection_h

#include "vtkExtractSelection.h"
#include "vtkPVVTKExtensionsExtractionModule.h" //needed for exports

class vtkSelectionNode;

class VTKPVVTKEXTENSIONSEXTRACTION_EXPORT vtkPVExtractSelection : public vtkExtractSelection
{
public:
  vtkTypeMacro(vtkPVExtractSelection, vtkExtractSelection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static const int OUTPUT_PORT_EXTRACTED_DATASET = 0;
  static const int OUTPUT_PORT_SELECTION_IDS = 1;
  static const int OUTPUT_PORT_SELECTION_ORIGINAL = 2;

  /**
   * Constructor
   */
  static vtkPVExtractSelection* New();

  /**
   * Removes all inputs from input port 1.
   */
  void RemoveAllSelectionsInputs() { this->SetInputConnection(1, 0); }

protected:
  vtkPVExtractSelection();
  ~vtkPVExtractSelection() override;

  // sets up empty output dataset
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // runs the algorithm and fills the output with results
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  vtkSelectionNode* LocateSelection(unsigned int level, unsigned int index, vtkSelection* sel);
  vtkSelectionNode* LocateSelection(unsigned int composite_index, vtkSelection* sel);

  /**
   * Creates a new vtkSelector for the given content type.
   * May return null if not supported. Overridden to handle
   * vtkSelectionNode::QUERY.
   */
  vtkSmartPointer<vtkSelector> NewSelectionOperator(
    vtkSelectionNode::SelectionContent type) override;

private:
  vtkPVExtractSelection(const vtkPVExtractSelection&) = delete;
  void operator=(const vtkPVExtractSelection&) = delete;

  class vtkSelectionNodeVector;
  void RequestDataInternal(
    vtkSelectionNodeVector& outputs, vtkDataObject* dataObjectOutput, vtkSelectionNode* sel);

  // Returns the combined content type for the selection.
  int GetContentType(vtkSelection* sel);
};

#endif
