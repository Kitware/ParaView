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
 *
 */

#ifndef vtkArithmeticAccumulator_h
#define vtkArithmeticAccumulator_h

#include "vtkAbstractAccumulator.h"

class vtkArithmeticAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkArithmeticAccumulator* New();

  vtkTypeMacro(vtkArithmeticAccumulator, vtkAbstractAccumulator);
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
  vtkArithmeticAccumulator();
  virtual ~vtkArithmeticAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated value
   */
  double Value;
  //@}

private:
  vtkArithmeticAccumulator(const vtkArithmeticAccumulator&) = delete;
  void operator=(const vtkArithmeticAccumulator&) = delete;
};

#endif
