/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStandardDeviationArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkStandardDeviationArrayMeasurement
 * @brief   measures the standard deviation of an array
 *
 * Measures the standard deviation of an array, either by giving the full array,
 * or by feeding value per value.
 * Merging complexity is constant, and overall algorithm is linear in function
 * of the input size.
 *
 */

#ifndef vtkStandardDeviationArrayMeasurement_h
#define vtkStandardDeviationArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class VTKCOMMONCORE_EXPORT vtkStandardDeviationArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkStandardDeviationArrayMeasurement* New();

  vtkTypeMacro(vtkStandardDeviationArrayMeasurement, vtkAbstractArrayMeasurement);
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
   * vtkStandardDeviationArrayMeasurement::MinimumNumberOfAccumulatedData accumulated data
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
  static const unsigned MinimumNumberOfAccumulatedData = 2;
  //@}

  //@{
  /**
   * Number of accumulators needed for measuring.
   */
  static const unsigned NumberOfAccumulators = 2;
  //@}

  //@{
  /**
   * Method for creating a vector composed of one vtkArithmeticAccumulator* followed
   * by one vtkSquaredArithmeticAccumulator*.
   */
  virtual std::vector<vtkAbstractAccumulator*> NewAccumulatorInstances() const override;
  //@}

  //@{
  /**
   * Method for measuring median using a vector of of accumulators.
   * The array should have the same dynamic types and size as the one returned by
   * vtkStandardDeviationArrayMeasurement::NewAccumulatorInstances().
   */
  virtual double Measure(
    const std::vector<vtkAbstractAccumulator*>&, vtkIdType numberOfAccumulatedData) const override;
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkStandardDeviationArrayMeasurement();
  virtual ~vtkStandardDeviationArrayMeasurement() override = default;
  //@}

private:
  vtkStandardDeviationArrayMeasurement(const vtkStandardDeviationArrayMeasurement&) = delete;
  void operator=(const vtkStandardDeviationArrayMeasurement&) = delete;
};

#endif
