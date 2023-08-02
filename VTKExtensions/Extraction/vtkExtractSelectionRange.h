// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractSelectionRange
 * @brief   compute the range of the input selection.
 *
 * vtkExtractSelectionRange is used to compute the range of an input selection.
 * This is an internal filter designed to be used to "compute visible range" for
 * a dataset.
 */

#ifndef vtkExtractSelectionRange_h
#define vtkExtractSelectionRange_h

#include "vtkPVVTKExtensionsExtractionModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class VTKPVVTKEXTENSIONSEXTRACTION_EXPORT vtkExtractSelectionRange : public vtkTableAlgorithm
{
public:
  static vtkExtractSelectionRange* New();
  vtkTypeMacro(vtkExtractSelectionRange, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the name of the array used to compute the range.
   */
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  ///@}

  ///@{
  /**
   * Set/Get the field type of the selection.
   */
  vtkSetMacro(FieldType, int);
  vtkGetMacro(FieldType, int);
  ///@}

  ///@{
  /**
   * Set/Get the component used to compute the range.
   */
  vtkSetMacro(Component, int);
  vtkGetMacro(Component, int);
  ///@}

  ///@{
  /**
   * Get the output range
   */
  vtkGetVector2Macro(Range, double);
  ///@}

protected:
  vtkExtractSelectionRange();
  ~vtkExtractSelectionRange() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FieldType;
  int Component;
  char* ArrayName;
  double Range[2];

private:
  vtkExtractSelectionRange(const vtkExtractSelectionRange&) = delete;
  void operator=(const vtkExtractSelectionRange&) = delete;
};

#endif
