/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedianArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkMedianArrayMeasurement
 * @brief   measures the median of an array
 *
 * Measures the entropy of an array, either by giving the full array,
 * or by feeding value per value.
 * Merging complexity is logarithmic as well as inserting, and overall algorithm is O(n*log(n)),
 * where n is the input size.
 *
 */

#ifndef vtkMedianArrayMeasurement_h
#define vtkMedianArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class VTKCOMMONCORE_EXPORT vtkMedianArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkMedianArrayMeasurement* New();

  vtkTypeMacro(vtkMedianArrayMeasurement, vtkAbstractArrayMeasurement);
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
  vtkMedianArrayMeasurement();
  virtual ~vtkMedianArrayMeasurement() override = default;
  //@}

private:
  vtkMedianArrayMeasurement(const vtkMedianArrayMeasurement&) = delete;
  void operator=(const vtkMedianArrayMeasurement&) = delete;
};

#endif
