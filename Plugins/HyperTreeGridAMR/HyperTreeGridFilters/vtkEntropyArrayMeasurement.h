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
 */

#ifndef vtkEntropyArrayMeasurement_h
#define vtkEntropyArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class VTKCOMMONCORE_EXPORT vtkEntropyArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkEntropyArrayMeasurement* New();

  vtkTypeMacro(vtkEntropyArrayMeasurement, vtkAbstractArrayMeasurement);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Returns the measure of input data.
   */
  virtual double Measure() const override;
  //@}

  //@{
  /**
   * Returns true if there is more than
   * vtkEntropyArrayMeasurement::MinimumNumberOfAccumulatedData accumulated data
   */
  virtual bool CanMeasure() const override;
  //@}

  //@{
  /**
   * Accessor for the minimum number of accumulated data necessary for computing the measure
   */
  virtual vtkIdType GetMinimumNumberOfAccumulatedData() const override;
  //@}

  //@{
  /**
   * Minimum number of accumulated data necessary to measure.
   */
  static const vtkIdType MinimumNumberOfAccumulatedData = 1;
  //@}

  //@{
  /**
   * Number of accumulators needed for measuring.
   */
  static const std::size_t NumberOfAccumulators = 2;
  //@}

  //@{
  /**
   * Method for creating a vector composed of one vtkMedianAccumulator*.
   */
  virtual std::vector<vtkAbstractAccumulator*> NewAccumulatorInstances() const override;
  //@}

  //@{
  /**
   * Method for measuring median using a vector of of accumulators.
   * The array should have the same dynamic types and size as the one returned by
   * vtkMedianArrayMeasurement::NewAccumulatorInstances().
   */
  virtual double Measure(
    const std::vector<vtkAbstractAccumulator*>&, vtkIdType numberOfAccumulatedData) const override;
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkEntropyArrayMeasurement();
  virtual ~vtkEntropyArrayMeasurement() override = default;
  //@}

private:
  vtkEntropyArrayMeasurement(const vtkEntropyArrayMeasurement&) = delete;
  void operator=(const vtkEntropyArrayMeasurement&) = delete;
};

#endif
