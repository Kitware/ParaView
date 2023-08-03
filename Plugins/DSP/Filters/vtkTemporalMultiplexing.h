// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkTemporalMultiplexing
 *
 * This filter creates a vtkTable containing 3D arrays based on a temporal input.
 * Each point/cell array of the input is converted to a 3D array defined by
 * (index, tuple, component), corresponding to (point/cell, timestep, component).
 * The arrays are vtkMultiDimensionalArray objects.
 */

#ifndef vtkTemporalMultiplexing_h
#define vtkTemporalMultiplexing_h

#include "vtkDSPFiltersPluginModule.h" // for export macro
#include "vtkNew.h"                    // for vtkNew
#include "vtkTableAlgorithm.h"

#include <memory> // for unique_ptr
#include <set>    // for std::set
#include <string> // for std::string

class vtkCompositeDataSet;
class vtkDataObject;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkTable;

class VTKDSPFILTERSPLUGIN_EXPORT vtkTemporalMultiplexing : public vtkTableAlgorithm
{
public:
  static vtkTemporalMultiplexing* New();
  vtkTypeMacro(vtkTemporalMultiplexing, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get the field association for arrays.
   * Default is point.
   */
  vtkSetMacro(FieldAssociation, int);
  vtkGetMacro(FieldAssociation, int);
  ///@}

  ///@{
  /**
   * Handle attribute arrays listing.
   */
  void EnableAttributeArray(const std::string& arrName);
  void ClearAttributeArrays();
  ///@}

protected:
  vtkTemporalMultiplexing();
  ~vtkTemporalMultiplexing() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkTemporalMultiplexing(const vtkTemporalMultiplexing&) = delete;
  void operator=(const vtkTemporalMultiplexing&) = delete;

  /**
   * Retrieve a valid point or cell data as well as the total number of points
   * or cells from the input depending on the current field association.
   */
  void GetArraysInformation(
    vtkDataObject* input, vtkSmartPointer<vtkDataSetAttributes>& attributes, vtkIdType& nbArrays);

  /**
   * Create and allocate vectors of arrays based on the given point or cell data.
   * Each vector has a number of elements matching the number of points or cells.
   * Each array in a vector has a number of tuples equal to the number of input
   * timesteps.
   */
  void PrepareVectorsOfArrays(
    vtkSmartPointer<vtkDataSetAttributes>& attributes, vtkIdType nbArrays);

  ///@{
  /**
   * Retrieve the values for each array at the current timestep and store them in
   * the corresponding vectors of arrays.
   */
  void FillArraysForCurrentTimestep(vtkDataSet* inputDS);
  void FillArraysForCurrentTimestep(vtkCompositeDataSet* inputCDS);
  ///@}

  /**
   * Create multi dimensional arrays once all timesteps have been processed.
   * Should run after the last call to RequestData().
   */
  void CreateMultiDimensionalArrays(vtkTable* output);

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  int NumberOfTimeSteps = 0;
  int CurrentTimeIndex = 0;
  int FieldAssociation = 0;
  std::set<std::string> SelectedArrays;
};

#endif // vtkTemporalMultiplexing_h
