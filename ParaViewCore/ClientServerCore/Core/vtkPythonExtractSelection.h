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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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
  ~vtkPythonExtractSelection();

  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Method used to initialize the output data object in request data.
   * The output data is initialized based on the state of
   * this->PreserveTopology.
   */
  void InitializeOutput(vtkDataObject* output, vtkDataObject* input);

private:
  vtkPythonExtractSelection(const vtkPythonExtractSelection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPythonExtractSelection&) VTK_DELETE_FUNCTION;
};

#endif
