/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedianAccumulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkMedianAccumulator
 * @brief   accumulates input data in a sorted array
 *
 * Accumulator for computing the median of the input data.
 * Inserting data is logarithmic in function of the input size,
 * while merging has a linear complexity.
 * Accessing the median from accumulated data has constant complexity
 *
 */

#ifndef vtkMedianAccumulator_h
#define vtkMedianAccumulator_h

#include "vtkAbstractAccumulator.h"

#include <vector>

class vtkMedianAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkMedianAccumulator* New();

  vtkTypeMacro(vtkMedianAccumulator, vtkAbstractAccumulator);
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
  virtual double GetValue() const override;
  //@}

  //@{
  /**
   * Set object into initial state
   */
  virtual void Initialize() override;
  //@}

  //@{
  /**
   * Getter of internally stored sorted list of values
   */
  std::vector<double>& GetSortedList() { return this->SortedList; }
  const std::vector<double>& GetSortedList() const { return this->SortedList; }
  //@}

protected:
  //@{
  /**
   * Default constructor and destructor.
   */
  vtkMedianAccumulator() = default;
  virtual ~vtkMedianAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated sorted list of values
   */
  std::vector<double> SortedList;
  //@}

private:
  vtkMedianAccumulator(const vtkMedianAccumulator&) = delete;
  void operator=(const vtkMedianAccumulator&) = delete;
};

#endif
