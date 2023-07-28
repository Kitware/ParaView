// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkMaxAccumulator
 * @brief   accumulates input data and delivers the maximum entry.
 *
 * Accumulator for computing the maximum of all inputs.
 */

#ifndef vtkMaxAccumulator_h
#define vtkMaxAccumulator_h

#include "vtkAbstractAccumulator.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkMaxAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkMaxAccumulator* New();

  vtkTemplateTypeMacro(vtkMaxAccumulator, vtkAbstractAccumulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using Superclass::Add;

  ///@{
  /**
   * Methods for adding data to the accumulator.
   */
  void Add(vtkAbstractAccumulator* accumulator) override;
  void Add(double value, double weight) override;
  ///@}

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
  void ShallowCopy(vtkObject* accumulator) override;

  /**
   * Deep copy of the accumulator.
   */
  void DeepCopy(vtkObject* accumulator) override;

  /**
   * Returns true if the parameters of accumulator is the same as the ones of this
   */
  bool HasSameParameters(vtkAbstractAccumulator* accumulator) const override;

protected:
  ///@{
  /**
   * Default constructor and destructor.
   */
  vtkMaxAccumulator();
  ~vtkMaxAccumulator() override = default;
  ///@}

  /**
   * Accumulated value
   */
  double Value;

private:
  vtkMaxAccumulator(vtkMaxAccumulator&) = delete;
  void operator=(vtkMaxAccumulator&) = delete;
};

#endif
