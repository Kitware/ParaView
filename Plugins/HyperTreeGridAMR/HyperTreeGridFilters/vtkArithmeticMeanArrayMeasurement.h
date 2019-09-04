/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArithmeticMeanArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkArithmeticMeanArrayMeasurement
 * @brief   measures the arithmetic mean of an array
 *
 * Measures the arithmetic mean of an array, either by giving the full array,
 * or by feeding value per value.
 * Merging complexity is constant, and overall algorithm is linear in function
 * of the input size.
 *
 */

#ifndef vtkArithmeticMeanArrayMeasurement_h
#define vtkArithmeticMeanArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

#include <vector>

class vtkAbstractAccumulator;

class VTKCOMMONCORE_EXPORT vtkArithmeticMeanArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkArithmeticMeanArrayMeasurement* New();

  vtkTypeMacro(vtkArithmeticMeanArrayMeasurement, vtkAbstractArrayMeasurement);
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
   * vtkArithmeticMeanArrayMeasurement::MinimumNumberOfAccumulatedData accumulated data
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
  static const unsigned MinimumNumberOfAccumulatedData = 1;
  //@}

  //@{
  /**
   * Number of accumulators needed for measuring.
   */
  static const unsigned NumberOfAccumulators = 1;
  //@}

  //@{
  /**
   * Method for creating a vector composed of one vtkArithmeticAccumulator*.
   */
  virtual std::vector<vtkAbstractAccumulator*> NewAccumulatorInstances() const override;
  //@}

  //@{
  /**
   * Method for measuring arithmetic mean using a vector of of accumulators.
   * The array should have the same dynamic types and size as the one returned by
   * vtkArithmeticMeanArrayMeasurement::NewAccumulatorInstances().
   */
  virtual double Measure(
    const std::vector<vtkAbstractAccumulator*>&, vtkIdType numberOfAccumulatedData) const override;
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkArithmeticMeanArrayMeasurement();
  virtual ~vtkArithmeticMeanArrayMeasurement() override = default;
  //@}

private:
  vtkArithmeticMeanArrayMeasurement(const vtkArithmeticMeanArrayMeasurement&) = delete;
  void operator=(const vtkArithmeticMeanArrayMeasurement&) = delete;
};

#endif
