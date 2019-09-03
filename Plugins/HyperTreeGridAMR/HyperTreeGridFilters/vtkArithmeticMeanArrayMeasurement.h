/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArithmeticMeanArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkArithmeticMeanArrayMeasurement
 * @brief   measures the arithmetic mean of an array
 *
 * Measures the arithmetic mean of an array, either by giving the full array,
 * or by feeding value per value.
 * Merging complexity is constant, and overall algorithm is linear in function
 * of the input size.
 *
 */

#ifndef vtkArithmeticMeanArrayMeasurement_h
#define vtkArithmeticMeanArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class VTKCOMMONCORE_EXPORT vtkArithmeticMeanArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkArithmeticMeanArrayMeasurement* New();

  vtkTypeMacro(vtkArithmeticMeanArrayMeasurement, vtkAbstractArrayMeasurement);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Returns the measure of input data.
   */
  virtual double Measure() const override;
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkArithmeticMeanArrayMeasurement();
  virtual ~vtkArithmeticMeanArrayMeasurement() override = default;
  //@}

private:
  vtkArithmeticMeanArrayMeasurement(const vtkArithmeticMeanArrayMeasurement&) = delete;
  void operator=(const vtkArithmeticMeanArrayMeasurement&) = delete;
};

#endif
