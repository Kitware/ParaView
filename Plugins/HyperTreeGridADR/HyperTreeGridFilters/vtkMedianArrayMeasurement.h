/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedianArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkMedianArrayMeasurement
 * @brief   measures the median of an array
 *
 * Measures the entropy of an array, either by giving the full array,
 * or by feeding value per value.
 * Merging complexity is logarithmic as well as inserting, and overall algorithm is O(n*log(n)),
 * where n is the input size.
 *
 */

#ifndef vtkMedianArrayMeasurement_h
#define vtkMedianArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkMedianArrayMeasurement
  : public vtkAbstractArrayMeasurement
{
public:
  static vtkMedianArrayMeasurement* New();

  vtkTypeMacro(vtkMedianArrayMeasurement, vtkAbstractArrayMeasurement);
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
  vtkMedianArrayMeasurement();
  virtual ~vtkMedianArrayMeasurement() override = default;
  //@}

private:
  vtkMedianArrayMeasurement(const vtkMedianArrayMeasurement&) = delete;
  void operator=(const vtkMedianArrayMeasurement&) = delete;
};

#endif
