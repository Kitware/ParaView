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
 * sum_i (w_i Functor(x_i)), where Functor is a function pointer.
 * The default value for Functor is VTK_FUNC_X from header vtkFunctorList.h,
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

template <typename FunctorT>
class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkArithmeticAccumulator : public vtkAbstractAccumulator
{
public:
  typedef FunctorT FunctorType;

  static vtkArithmeticAccumulator* New();

  vtkTemplateTypeMacro(vtkArithmeticAccumulator, vtkAbstractAccumulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using Superclass::Add;

  //@{
  /**
   * Methods for adding data to the accumulator.
   */
  void Add(vtkAbstractAccumulator* accumulator) override;
  void Add(double value, double weight) override;
  //@}

  /**
   * Accessor to the accumulated value.
   */
  double GetValue() const override;

  /**
   * Set object into initial state
   */
  void Initialize() override;

  /**
   * Shallow copy of the accumulator.
   */
  void ShallowCopy(vtkDataObject* accumulator) override;

  /**
   * Deep copy of the accumulator.
   */
  void DeepCopy(vtkDataObject* accumulator) override;

  /**
   * Returns true if the parameters of accumulator is the same as the ones of this
   */
  bool HasSameParameters(vtkAbstractAccumulator* accumulator) const override;

  //@{
  /**
   * Accessor/mutator on the function pointer specifying which function is applied to
   * the data to accumulate.
   * The accumulated value is sum_i w_i Functor(x_i), where x_i is the input data,
   * and w_i the corresponding weight.
   */
  const FunctorT& GetFunctor() const;
  void SetFunctor(const FunctorT&& f);
  //@}

protected:
  //@{
  /**
   * Default constructor and destructor.
   */
  vtkArithmeticAccumulator();
  ~vtkArithmeticAccumulator() override = default;
  //@}

  /**
   * Accumulated value
   */
  double Value;

  /**
   * Function applied to the values to accumulate.
   */
  FunctorType Functor;

private:
  vtkArithmeticAccumulator(vtkArithmeticAccumulator<FunctorT>&) = delete;
  void operator=(vtkArithmeticAccumulator<FunctorT>&) = delete;
};

#include "vtkArithmeticAccumulator.txx"
#endif
