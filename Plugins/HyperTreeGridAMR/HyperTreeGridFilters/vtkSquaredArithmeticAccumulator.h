/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSquaredArithmeticAccumulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkSquaredArithmeticAccumulator
 * @brief   accumulates input data by summing the squared input
 *
 * Accumulator for adding arithmetically the squared input.
 * The resulting accumulated value is the sum of the squared value of each input element.
 */

#ifndef vtkSquaredArithmeticAccumulator_h
#define vtkSquaredArithmeticAccumulator_h

#include "vtkAbstractAccumulator.h"

class vtkSquaredArithmeticAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkSquaredArithmeticAccumulator* New();

  vtkTypeMacro(vtkSquaredArithmeticAccumulator, vtkAbstractAccumulator);
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
  vtkSquaredArithmeticAccumulator();
  virtual ~vtkSquaredArithmeticAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated value
   */
  double Value;
  //@}

private:
  vtkSquaredArithmeticAccumulator(const vtkSquaredArithmeticAccumulator&) = delete;
  void operator=(const vtkSquaredArithmeticAccumulator&) = delete;
};

#endif
