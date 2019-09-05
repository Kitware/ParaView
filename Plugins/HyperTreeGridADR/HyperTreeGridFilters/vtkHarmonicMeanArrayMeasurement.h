/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHarmonicMeanArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

  //@{
  /**
   * Returns the measure of input data.
   */
  virtual double Measure() const override;
  //@}

  //@{
  /**
   * Returns true if there is more than
   * vtkHarmonicMeanArrayMeasurement::MinimumNumberOfAccumulatedData accumulated data
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
   * Method for creating a vector composed of one vtkInversedArithmeticAccumulator*.
   */
  virtual std::vector<vtkAbstractAccumulator*> NewAccumulatorInstances() const override;
  //@}

  //@{
  /**
   * Method for measuring harmonic mean using a vector of of accumulators.
   * The array should have the same dynamic types and size as the one returned by
   * vtkHarmonicMeanArrayMeasurement::NewAccumulatorInstances().
   */
  virtual double Measure(
    const std::vector<vtkAbstractAccumulator*>&, vtkIdType numberOfAccumulatedData) const override;
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkHarmonicMeanArrayMeasurement();
  virtual ~vtkHarmonicMeanArrayMeasurement() override = default;
  //@}

private:
  vtkHarmonicMeanArrayMeasurement(const vtkHarmonicMeanArrayMeasurement&) = delete;
  void operator=(const vtkHarmonicMeanArrayMeasurement&) = delete;
};

#endif
