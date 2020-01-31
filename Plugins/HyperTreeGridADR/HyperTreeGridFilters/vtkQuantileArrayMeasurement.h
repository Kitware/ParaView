/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuantileArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkQuantileArrayMeasurement
 * @brief   measures the quantile of an array
 *
 * Measures the quantile of an array, either by giving the full array,
 * or by feeding value per value. The user sets the Percentile, which is not necessary an integer.
 *
 * Merging complexity is logarithmic as well as inserting, and overall algorithm is O(n*log(n)),
 * where n is the input size.
 *
 * Measuring accumulated data has a constant complexity.
 *
 * @note If one wants to compute the median, one should call
 * vtkQuantileArrayMeasurement::SetPercentile(50). If one wants to compute the first quartile
 * instead, one should call vtkQuantileArrayMeasurement::SetPercentile(25). etc.
 */

#ifndef vtkQuantileArrayMeasurement_h
#define vtkQuantileArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkQuantileArrayMeasurement
  : public vtkAbstractArrayMeasurement
{
public:
  static vtkQuantileArrayMeasurement* New();

  vtkTypeMacro(vtkQuantileArrayMeasurement, vtkAbstractArrayMeasurement);

  using Superclass::Add;
  using Superclass::CanMeasure;
  using Superclass::Measure;

  /**
   * Minimum times the function Add should be called on accumulators or this class
   * in order to measure something.
   */
  static constexpr vtkIdType MinimumNumberOfAccumulatedData = 1;

  /**
   * Number of accumulators required for measuring.
   */
  static constexpr vtkIdType NumberOfAccumulators = 1;

  /**
   * Notifies if the quantile can be measured given the amount of input data.
   * The quantile needs at least one accumulated data with non-zero weight.
   *
   * @param numberOfAccumulatedData is the number of times Add was called in the accumulators / this
   * class
   * @param totalWeight is the accumulated weight while accumulated. If weights were not set when
   * accumulated, it should be equal to numberOfAccumulatedData.
   * @return true if there is enough data and if totalWeight != 0, false otherwise.
   */
  static bool IsMeasurable(vtkIdType numberOfAccumulatedData, double totalWeight);

  /**
   * Instantiates needed accumulators for measurement, i.e. one vtkQuantileAccumulator* in our case.
   *
   * @return the array {vtkQuantileAccumulator::New()}.
   */
  static std::vector<vtkAbstractAccumulator*> NewAccumulators();

  /**
   * Computes the quantile of the set of accumulators needed (i.e. one vtkQuantileAccumulator*).
   *
   * @param accumulators is an array of accumulators. It should be composed of a single
   * vtkQuantileAccumulator*.
   * @param numberOfAccumulatedData is the number of times the method Add was called in the
   * accumulators.
   * @param totalWeight is the cumulated weight when adding data. If weight was not set while
   * accumulating. it should equal numberOfAccumulatedData.
   * @param value is where the quantile measurement is written into.
   * @return true if the data is measurable i.e. there is not enough data or totalWeight is null.
   */
  bool Measure(vtkAbstractAccumulator** accumulators, vtkIdType numberOfAccumulatedData,
    double totalWeight, double& value) override;

  //@{
  /**
   * See the vtkAbstractArrayMeasurement API for description of this method.
   */
  bool CanMeasure(vtkIdType numberOfAccumulatedData, double totalWeight) const override;
  std::vector<vtkAbstractAccumulator*> NewAccumulatorInstances() const override;
  vtkIdType GetMinimumNumberOfAccumulatedData() const override;
  vtkIdType GetNumberOfAccumulators() const override;
  //@}

  /**
   * ShallowCopy implementation.
   */
  void ShallowCopy(vtkDataObject* o) override;

  /**
   * DeepCopy implementation.
   */
  void DeepCopy(vtkDataObject* o) override;

  //@{
  /**
   * Set/Get macros to Percentile to measure. Note that it does not need to be an integer.
   *
   * @note Setting Percentile to 50 is equivalent with computing the median.
   */
  double GetPercentile() const;
  void SetPercentile(double percentile);
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkQuantileArrayMeasurement();
  ~vtkQuantileArrayMeasurement() override = default;
  //@}

private:
  vtkQuantileArrayMeasurement(vtkQuantileArrayMeasurement&) = delete;
  void operator=(vtkQuantileArrayMeasurement&) = delete;
};

#endif
