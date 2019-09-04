/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEntropyAccumulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkEntropyAccumulator
 * @brief   accumulates input data by summing the input elements times their logarithm
 *
 * Accumulator for computing the entropy of the input data.
 * The resulting accumulated value is the sum of the input element weighted by their logarithm.
 */

#ifndef vtkEntropyAccumulator_h
#define vtkEntropyAccumulator_h

#include "vtkAbstractAccumulator.h"

class VTKCOMMONCORE_EXPORT vtkEntropyAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkEntropyAccumulator* New();

  vtkTypeMacro(vtkEntropyAccumulator, vtkAbstractAccumulator);
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
  vtkEntropyAccumulator();
  virtual ~vtkEntropyAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated value
   */
  double Value;
  //@}

private:
  vtkEntropyAccumulator(const vtkEntropyAccumulator&) = delete;
  void operator=(const vtkEntropyAccumulator&) = delete;
};

#endif
