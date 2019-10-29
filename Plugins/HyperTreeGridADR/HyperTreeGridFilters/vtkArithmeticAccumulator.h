/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArithmeticAccumulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkArithmeticAccumulator
 * @brief   accumulates input data arithmetically
 *
 * Accumulator for adding arithmetically data.
 * The resulting accumulated value is the sum of all the inputs.
 * Noting x_i the set of inputs and w_i its weight, the accumulated value would be
 * sum_i (w_i FunctionOfX(x_i)), where FunctionOfX is a function pointer.
 * The default value for FunctionOfX is VTK_FUNC_X from header vtkFunctionOfXList.h,
 * which is the identity function.
 *
 * @note Arithmetic accumulators can be used to accumulate using the product instead of the sum.
 * One need to arithmetically accumulate logarithm of the input, and then compute the exponential.
 *
 */

#ifndef vtkArithmeticAccumulator_h
#define vtkArithmeticAccumulator_h

#include "vtkAbstractAccumulator.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

#include <cassert>
#include <functional>
#include <string>
#include <unordered_map>

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkArithmeticAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkArithmeticAccumulator* New();

  vtkTemplateTypeMacro(vtkArithmeticAccumulator, vtkAbstractAccumulator);
  virtual void PrintSelf(ostream& os, vtkIndent indent) override;

  using Superclass::Add;

  //@{
  /**
   * Methods for adding data to the accumulator.
   */
  virtual void Add(vtkAbstractAccumulator* accumulator) override;
  virtual void Add(double value, double weight) override;
  //@}

  /**
   * Accessor to the accumulated value.
   */
  virtual double GetValue() const override;

  /**
   * Set object into initial state
   */
  virtual void Initialize() override;

  /**
   * Shallow copy of the accumulator.
   */
  virtual void ShallowCopy(vtkDataObject* accumulator) override;

  /**
   * Deep copy of the accumulator.
   */
  virtual void DeepCopy(vtkDataObject* accumulator) override;

  /**
   * Returns true if the parameters of accumulator is the same as the ones of this
   */
  virtual bool HasSameParameters(vtkAbstractAccumulator* accumulator) const override;

  //@{
  /**
   * Accessor/mutator on the function pointer specifying which function is applied to
   * the data to accumulate.
   * The accumulated value is sum_i w_i FunctionOfX(x_i), where x_i is the input data,
   * and w_i the corresponding weight.
   */
  const std::function<double(double)>& GetFunctionOfX() const;
  void SetFunctionOfX(double (*const f)(double), const std::string& name);
  void SetFunctionOfX(const std::function<double(double)>& f, const std::string& name);
  //@}

protected:
  //@{
  /**
   * Default constructor and destructor.
   */
  vtkArithmeticAccumulator();
  virtual ~vtkArithmeticAccumulator() override = default;
  //@}

  /**
   * Accumulated value
   */
  double Value;

  /**
   * Function applied to the values to accumulate.
   */
  std::function<double(double)> FunctionOfX;

  /**
   * Storage of function name.
   */
  static std::unordered_map<double (*)(double), std::string> FunctionName;

private:
  vtkArithmeticAccumulator(vtkArithmeticAccumulator&) = delete;
  void operator=(vtkArithmeticAccumulator&) = delete;
};

#endif
