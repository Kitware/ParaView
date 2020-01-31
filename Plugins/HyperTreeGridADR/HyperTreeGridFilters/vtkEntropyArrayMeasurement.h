/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEntropyArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkEntropyArrayMeasurement
 * @brief   measures the entropy of an array
 *
 * Measures the entropy of an array, either by giving the full array,
 * or by feeding value per value.
 * Merging complexity is constant, and overall algorithm is linear in function
 * of the input size.
 *
 * @warning Unlike most vtkAbstractArrayMeasurement, given accumulated data,
 * the entropy complexity is linear in function of the number of bins.
 */

#ifndef vtkEntropyArrayMeasurement_h
#define vtkEntropyArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkEntropyArrayMeasurement
  : public vtkAbstractArrayMeasurement
{
public:
  static vtkEntropyArrayMeasurement* New();

  vtkTypeMacro(vtkEntropyArrayMeasurement, vtkAbstractArrayMeasurement);

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
   * Notifies if the entropy can be measured given the amount of input data.
   * The entropy needs at least one accumulated data with non-zero weight.
   *
   * @param numberOfAccumulatedData is the number of times Add was called in the accumulators / this
   * class
   * @param totalWeight is the accumulated weight while accumulated. If weights were not set when
   * accumulated, it should be equal to numberOfAccumulatedData.
   * @return true if there is enough data and if totalWeight != 0, false otherwise.
   */
  static bool IsMeasurable(vtkIdType numberOfAccumulatedData, double totalWeight);

  /**
   * Instantiates needed accumulators for measurement, i.e. one vtkBinsAccumulator* in our case.
   *
   * @return the array {vtkBinsAccumulator::New()}.
   */
  static std::vector<vtkAbstractAccumulator*> NewAccumulators();

  /**
   * Computes the entropy of the set of accumulators needed (i.e. one vtkBinsAccumulator*).
   *
   * @param accumulators is an array of accumulators. It should be composed of a single
   * vtkBinsAccumulator*.
   * @param numberOfAccumulatedData is the number of times the method Add was called in the
   * accumulators.
   * @param totalWeight is the cumulated weight when adding data. If weight was not set while
   * accumulating. it should equal numberOfAccumulatedData.
   * @param value is where the entropy measurement is written into.
   * @return true if the data is measurable, i.e. there is not enough data or a null totalWeight.
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

  //@{
  /**
   * Accessors on the DiscretizationStep used for calculating the entropy.
   */
  double GetDiscretizationStep() const;
  void SetDiscretizationStep(double);
  //@}

protected:
  //@{
  /**
   * Default constructor and destructor
   */
  vtkEntropyArrayMeasurement();
  ~vtkEntropyArrayMeasurement() override = default;
  //@}

private:
  vtkEntropyArrayMeasurement(vtkEntropyArrayMeasurement&) = delete;
  void operator=(vtkEntropyArrayMeasurement&) = delete;
};

#endif
