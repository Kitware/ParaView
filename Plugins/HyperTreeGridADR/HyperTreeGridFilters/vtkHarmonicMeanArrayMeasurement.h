// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkHarmonicMeanArrayMeasurement
 * @brief   measures the harmonic mean of an array
 *
 * Measures the harmonic mean of an array, either by giving the full array,
 * or by feeding value per value.
 * Merging complexity is constant, and overall algorithm is linear in function
 * of the input size.
 *
 * @warning The harmonic mean of data with zero values is undefined
 */

#ifndef vtkHarmonicMeanArrayMeasurement_h
#define vtkHarmonicMeanArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkHarmonicMeanArrayMeasurement
  : public vtkAbstractArrayMeasurement
{
public:
  static vtkHarmonicMeanArrayMeasurement* New();

  vtkTypeMacro(vtkHarmonicMeanArrayMeasurement, vtkAbstractArrayMeasurement);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * Notifies if the arithmetic mean can be measured given the amount of input data.
   * The arithmetic mean needs at least one accumulated data with non-zero weight.
   *
   * @param numberOfAccumulatedData is the number of times Add was called in the accumulators / this
   * class
   * @param totalWeight is the accumulated weight while accumulated. If weights were not set when
   * accumulated, it should be equal to numberOfAccumulatedData.
   * @return true if there is enough data and if totalWeight != 0, false otherwise.
   */
  static bool IsMeasurable(vtkIdType numberOfAccumulatedData, double totalWeight);

  /**
   * Instantiates needed accumulators for measurement, i.e. one vtkIdentityArithmeticAccumulator* in
   * our case.
   *
   * @return the array {vtkIdentityArithmeticAccumulator::New()}.
   */
  static std::vector<vtkAbstractAccumulator*> NewAccumulators();

  /**
   * Computes the arithmetic mean of the set of accumulators needed (i.e. one
   * vtkIdentityArithmeticAccumulator*).
   *
   * @param accumulators is an array of accumulators. It should be composed of a
   * single vtkIdentityArithmeticAccumulator*.
   * @param numberOfAccumulatedData is the number of times the method Add was called in the
   * accumulators.
   * @param totalWeight is the cumulated weight when adding data. If weight was not set while
   * accumulating. it should equal numberOfAccumulatedData.
   * @param value is where the arithmetic mean measurement is written into.
   * @return true if the data is measurable, i.e. there is not enough data or a null totalWeight.
   */
  bool Measure(vtkAbstractAccumulator** accumulators, vtkIdType numberOfAccumulatedData,
    double totalWeight, double& value) override;

  ///@{
  /**
   * See the vtkAbstractArrayMeasurement API for description of this method.
   */
  bool CanMeasure(vtkIdType numberOfAccumulatedData, double totalWeight) const override;
  std::vector<vtkAbstractAccumulator*> NewAccumulatorInstances() const override;
  vtkIdType GetMinimumNumberOfAccumulatedData() const override;
  vtkIdType GetNumberOfAccumulators() const override;
  ///@}

protected:
  ///@{
  /**
   * Default constructors and destructors
   */
  vtkHarmonicMeanArrayMeasurement();
  ~vtkHarmonicMeanArrayMeasurement() override = default;
  ///@}

private:
  vtkHarmonicMeanArrayMeasurement(const vtkHarmonicMeanArrayMeasurement&) = delete;
  void operator=(const vtkHarmonicMeanArrayMeasurement&) = delete;
};

#endif
