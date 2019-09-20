/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractAccumulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkAbstractAccumulator
 * @brief   accumulates input data given a certain pattern
 *
 * Given input values, this class accumulates data following a certain pattern.
 * This class is typically used for array measurement inside of
 * vtkAbstractArrayMeasurement. It allows to compute estimates
 * over the array while adding values into it on the fly,
 * with the ability of computing the wanted measure in O(1)
 * at any state of the accumulator.
 *
 * Unless specified otherwise, adding a value is constant in time, as well as merging
 * accumulators of the same type.
 *
 */

#ifndef vtkAbstractAccumulator_h
#define vtkAbstractAccumulator_h

#include "vtkDataObject.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

#include <functional>

class vtkDataArray;
class vtkDoubleArray;

#define vtkValueComaNameMacro(value) value, #value

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkAbstractAccumulator : public vtkDataObject
{
public:
  vtkAbstractTypeMacro(vtkAbstractAccumulator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkAbstractAccumulator* New();

  //@{
  /**
   * Methods for adding data to the accumulator.
   */
  virtual void Add(vtkDataArray* data, vtkDoubleArray* weights = nullptr);
  virtual void Add(const double* data, vtkIdType numberOfElements = 1, double weight = 1.0);
  virtual void Add(vtkAbstractAccumulator* accumulator) = 0;
  virtual void Add(double value, double weight) = 0;
  //@}

  //@{
  /**
   * Returns true if the parameters of accumulator is the same as the ones of this
   */
  virtual bool HasSameParameters(vtkAbstractAccumulator* accumulator) const = 0;
  //@}

  /**
   * Accessor on the accumulated value.
   */
  virtual double GetValue() const = 0;

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkAbstractAccumulator();
  virtual ~vtkAbstractAccumulator() override = default;
  //@}

  /**
   * Lambda expression converting vectors to scalars. Default function is regular L2 norm.
   */
  std::function<double(const double*, vtkIdType)> ConvertVectorToScalar;

private:
  vtkAbstractAccumulator(vtkAbstractAccumulator&) = delete;
  void operator=(vtkAbstractAccumulator&) = delete;
};

#endif
