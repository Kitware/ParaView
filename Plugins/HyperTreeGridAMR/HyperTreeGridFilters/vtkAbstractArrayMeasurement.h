/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkAbstractArrayMeasurement
 * @brief   measures quantities on arrays of values
 *
 * Given an array of data, it computes the wanted quantity over the array.
 * Data can be fed using the whole array as input, or value by value,
 * or by merging two vtkAbstractArrayMeasurement*.
 * The complexity depends on the array of vtkAbstractAccumulator*
 * used to compute the measurement. Given n inputs, if f(n) is the
 * worst complexity given used accumulators, the complexity of inserting
 * data is O(n f(n)), and the complexity for measuring already
 * inserted data is O(1), unless said otherwise.
 * Merging complexity depends on the vtkAbstractAccumulator* used.
 *
 */

#ifndef vtkAbstractArrayMeasurement_h
#define vtkAbstractArrayMeasurement_h

#include "vtkAbstractAccumulator.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObject.h"
#include "vtkSetGet.h"

#include <vector>

class VTKCOMMONCORE_EXPORT vtkAbstractArrayMeasurement : public vtkObject
{
public:
  static vtkAbstractArrayMeasurement* New();

  vtkAbstractTypeMacro(vtkAbstractArrayMeasurement, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Methods used to add data to the accumulators.
   */
  virtual void Add(vtkDataArray* data);
  virtual void Add(double* data, vtkIdType numberOfComponents = 1);
  virtual void Add(vtkAbstractArrayMeasurement* arrayMeasurement);
  //@}

  //@{
  /**
   * Returns true if the array has enough data to be measured.
   */
  virtual bool CanMeasure() const = 0;
  //@}

  //@{
  /**
   * Returns the measure of input data.
   */
  virtual double Measure() const = 0;
  //@}

  //@{
  /**
   * Returns a vector filled with one pointer of each needed accumulator for measuring.
   */
  virtual std::vector<vtkAbstractAccumulator*> NewAccumulatorInstances() const = 0;
  //@}

  //@{
  /**
   * Method for measuring any set of accumulators.
   */
  virtual double Measure(
    const std::vector<vtkAbstractAccumulator*>&, vtkIdType numberOfAccumulatedData) const = 0;
  //@}

  //@{
  /**
   * Accessor for the minimum number of accumulated data necessary for computing the measure
   */
  virtual vtkIdType GetMinimumNumberOfAccumulatedData() const = 0;
  //@}

  //@{
  /**
   * Set object into initial state.
   */
  virtual void Initialize();
  //@}

  //@{
  /**
   * Accessor for the number of values already fed for the measurement
   */
  vtkGetMacro(NumberOfAccumulatedData, vtkIdType);

  //@{
  /**
   * Number of accumulators needed for measurement
   */
  virtual std::size_t GetNumberOfAccumulators() const { return this->Accumulators.size(); }
  //@}

  //@{
  /**
   * Accessor for accumulators
   */
  virtual const std::vector<vtkAbstractAccumulator*>& GetAccumulators() const
  {
    return this->Accumulators;
  }
  virtual std::vector<vtkAbstractAccumulator*>& GetAccumulators() { return this->Accumulators; }
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkAbstractArrayMeasurement();
  virtual ~vtkAbstractArrayMeasurement() override;
  //@}

  //@{
  /**
   * Accumulators used to accumulate the input data.
   */
  std::vector<vtkAbstractAccumulator*> Accumulators;
  //@}

  //@{
  /**
   * Amount of data already fed to accumulators.
   */
  vtkIdType NumberOfAccumulatedData;
  //@}

private:
  vtkAbstractArrayMeasurement(const vtkAbstractArrayMeasurement&) = delete;
  void operator=(const vtkAbstractArrayMeasurement&) = delete;
};

#endif
