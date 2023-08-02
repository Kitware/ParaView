// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkAnnotateSelectionFilter
 * @brief filter for annotating selection
 *
 * vtkAnnotateSelectionFilter is a filter that can be used to generate a
 * string array based on a selection.
 *
 * The input selection is split for each selection node, we should specify the same number of
 * selection node than the number of label.
 */

#ifndef vtkAnnotateSelectionFilter_h
#define vtkAnnotateSelectionFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkAnnotateSelectionFilter
  : public vtkDataSetAlgorithm
{
public:
  static vtkAnnotateSelectionFilter* New();
  vtkTypeMacro(vtkAnnotateSelectionFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the label to display the input.
   *
   * @note We should specify the same number of label than the number of selection node inside the
   * input selection, if this is not the case the filter will fail and return an error.
   */
  void ResetLabels();
  void SetLabels(int index, const char* label);
  ///@}

  /**
   * Convenience method to specify the selection connection (2nd input
   * port)
   */
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

  /**
   * Removes all inputs from input port 1.
   */
  void RemoveAllSelectionsInputs() { this->SetInputConnection(1, nullptr); }

  ///@{
  /**
   * Set/Get the default label used for data that isn't labelized by a selection.
   */
  vtkSetStdStringFromCharMacro(DefaultLabel);
  vtkGetCharFromStdStringMacro(DefaultLabel);
  ///@}

  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkAnnotateSelectionFilter();
  ~vtkAnnotateSelectionFilter() override = default;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  std::vector<std::string> Labels;

  std::string DefaultLabel;

private:
  vtkAnnotateSelectionFilter(const vtkAnnotateSelectionFilter&) = delete;
  void operator=(const vtkAnnotateSelectionFilter&) = delete;
};

#endif
