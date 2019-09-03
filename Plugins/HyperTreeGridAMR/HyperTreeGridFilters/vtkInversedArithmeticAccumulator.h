/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInversedArithmeticAccumulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkInversedArithmeticAccumulator
 * @brief   accumulates input data by summing the inverse of the input
 *
 * Accumulator for adding arithmetically the inverse of the input.
 * The resulting accumulated value is the sum of the inverse of each input element.
 *
 * @warning One cannot add zero in this accumulator
 */

#ifndef vtkInversedArithmeticAccumulator_h
#define vtkInversedArithmeticAccumulator_h

#include "vtkAbstractAccumulator.h"

class vtkInversedArithmeticAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkInversedArithmeticAccumulator* New();

  vtkTypeMacro(vtkInversedArithmeticAccumulator, vtkAbstractAccumulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using Superclass::Add;

  //@{
  /**
   * Methods for adding data to the accumulator.
   */
  virtual void Add(vtkAbstractAccumulator* accumulator);
  virtual void Add(double value);
  //@}

  //@{
  /**
   * Accessor to the accumulated value.
   */
  virtual double GetValue() const override { return this->Value; }
  //@}

  //@{
  /**
   * Set object into initial state
   */
  virtual void Initialize() override;
  //@}

protected:
  //@{
  /**
   * Default constructor and destructor.
   */
  vtkInversedArithmeticAccumulator();
  virtual ~vtkInversedArithmeticAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated value
   */
  double Value;
  //@}

private:
  vtkInversedArithmeticAccumulator(const vtkInversedArithmeticAccumulator&) = delete;
  void operator=(const vtkInversedArithmeticAccumulator&) = delete;
};

#endif
