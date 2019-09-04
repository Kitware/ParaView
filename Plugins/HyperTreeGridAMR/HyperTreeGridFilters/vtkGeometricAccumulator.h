/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeometricAccumulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGeometricAccumulator
 * @brief   accumulates input data geometrically
 *
 * Accumulator for adding geometrically data.
 * The resulting accumulated value is the product of all the inputs.
 *
 * @warning One cannot geometrically accumulate zero or negative data.
 */

#ifndef vtkGeometricAccumulator_h
#define vtkGeometricAccumulator_h

#include "vtkAbstractAccumulator.h"

class VTKCOMMONCORE_EXPORT vtkGeometricAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkGeometricAccumulator* New();

  vtkTypeMacro(vtkGeometricAccumulator, vtkAbstractAccumulator);
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
  vtkGeometricAccumulator();
  virtual ~vtkGeometricAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated value
   */
  double Value;
  //@}

private:
  vtkGeometricAccumulator(const vtkGeometricAccumulator&) = delete;
  void operator=(const vtkGeometricAccumulator&) = delete;
};

#endif
