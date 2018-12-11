/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPythonExtractSelection
 *
 * vtkPythonExtractSelection is a used to extra cells/points using numpy. This
 * enables creation of arbitrary queries to be used as the selection criteria.
*/

#ifndef vtkPythonExtractSelection_h
#define vtkPythonExtractSelection_h

#include "vtkExtractSelectionBase.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports

class vtkCompositeDataSet;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPythonExtractSelection : public vtkExtractSelectionBase
{
public:
  static vtkPythonExtractSelection* New();
  vtkTypeMacro(vtkPythonExtractSelection, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Method called by Python code to handle the extraction logic.
   * \c attributeType is vtkDataObject::AttributeTypes and not to be confused with
   * vtkSelectionNode::SelectionField
   */
  bool ExtractElements(int attributeType, vtkDataObject* input, vtkDataObject* output);
  bool ExtractElements(int attributeType, vtkCompositeDataSet* input, vtkCompositeDataSet* output);
  //@}

protected:
  vtkPythonExtractSelection();
  ~vtkPythonExtractSelection() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Method used to initialize the output data object in request data.
   * The output data is initialized based on the state of
   * this->PreserveTopology.
   */
  void InitializeOutput(vtkDataObject* output, vtkDataObject* input);

private:
  vtkPythonExtractSelection(const vtkPythonExtractSelection&) = delete;
  void operator=(const vtkPythonExtractSelection&) = delete;
};

#endif
