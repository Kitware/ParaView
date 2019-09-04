/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometricMeanArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGeometricMeanArrayMeasurement
 * @brief   measures the geometric mean of an array
 *
 * Measures the geometric mean of an array, either by giving the full array,
 * or by feeding value per value.
 * Merging complexity is constant, and overall algorithm is linear in function
 * of the input size.
 *
 * @warning The geometric mean of data with negative or zero values is undefined
 */

#ifndef vtkGeometricMeanArrayMeasurement_h
#define vtkGeometricMeanArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class VTKCOMMONCORE_EXPORT vtkGeometricMeanArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkGeometricMeanArrayMeasurement* New();

  vtkTypeMacro(vtkGeometricMeanArrayMeasurement, vtkAbstractArrayMeasurement);
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
   * vtkGeometricMeanArrayMeasurement::MinimumNumberOfAccumulatedData accumulated data
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
   * Method for creating a vector composed of one vtkGeometricAccumulator*.
   */
  virtual std::vector<vtkAbstractAccumulator*> NewAccumulatorInstances() const override;
  //@}

  //@{
  /**
   * Number of accumulators needed for measuring.
   */
  static const unsigned NumberOfAccumulators = 1;
  //@}

  //@{
  /**
   * Method for measuring geometric mean using a vector of of accumulators.
   * The array should have the same dynamic types and size as the one returned by
   * vtkGeometricMeanArrayMeasurement::NewAccumulatorInstances().
   */
  virtual double Measure(
    const std::vector<vtkAbstractAccumulator*>&, vtkIdType numberOfAccumulatedData) const override;
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkGeometricMeanArrayMeasurement();
  virtual ~vtkGeometricMeanArrayMeasurement() override = default;
  //@}

private:
  vtkGeometricMeanArrayMeasurement(const vtkGeometricMeanArrayMeasurement&) = delete;
  void operator=(const vtkGeometricMeanArrayMeasurement&) = delete;
};

#endif
