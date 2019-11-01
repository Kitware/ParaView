/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBinsAccumulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkBinsAccumulator
 * @brief   accumulates input data arithmetically
 *
 * Accumulator for adding arithmetically data.
 * The resulting accumulated value is the sum of all the inputs.
 *
 * The user should set the function pointer FunctionOfW, which is by default the identity.
 * A collection of usable functions can be find in header vtkFunctionOfXList.h.
 * FunctionOfW is used as follows: the accumulated value equals sum_i FunctionOfW(w_i).
 * where w_i is the accumulated weight at bin i.
 *
 * One can use this accumulator to compute the entropy by setting FunctionOfW = VTK_FUNC_XLOGX
 * if the header vtkFunctionOfXList.h is included. Normalizing the result shall be performed.
 */

#ifndef vtkBinsAccumulator_h
#define vtkBinsAccumulator_h

#include "vtkAbstractAccumulator.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

#include <memory>
#include <string>
#include <unordered_map>

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkBinsAccumulator : public vtkAbstractAccumulator
{
public:
  typedef std::unordered_map<long long, double> BinsType;
  typedef std::shared_ptr<BinsType> BinsPointer;

  static vtkBinsAccumulator* New();

  vtkTypeMacro(vtkBinsAccumulator, vtkAbstractAccumulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using Superclass::Add;

  //@{
  /**
   * Methods for adding data to the accumulator
   */
  virtual void Add(vtkAbstractAccumulator* accumulator) override;
  virtual void Add(double value, double weight = 1.0) override;
  //@}

  /**
   * Clears the bins.
   */
  virtual void Initialize() override;

  /**
   * Accessor to the accumulated Bins
   */
  const BinsPointer GetBins() const;

  //@{
  /**
   * Accessor to the discretization step. This sets the Bins widths.
   */
  vtkGetMacro(DiscretizationStep, double);
  void SetDiscretizationStep(double);
  //@}

  /**
   * ShallowCopy implementation, both object then share the same Bins.
   */
  virtual void DeepCopy(vtkDataObject* accumulator) override;

  /**
   * DeepCopy implementation.
   */
  virtual void ShallowCopy(vtkDataObject* accumulator) override;

  /**
   * Returns true if the parameters of accumulator is the same as the ones of this
   */
  virtual bool HasSameParameters(vtkAbstractAccumulator* accumulator) const override;

  //@{
  /**
   * Accessor/mutator on the function pointer specifying which quantity should be computed on the
   * bins. Bins are filled with the accumulated weight of the corresponding value range. The
   * accumulated value is sum_i FunctionOfW(w_i), where w_i is the weight at bin i.
   */
  const std::function<double(double)>& GetFunctionOfW() const;
  void SetFunctionOfW(double (*const f)(double), const std::string& name);
  void SetFunctionOfW(const std::function<double(double)>& f, const std::string& name);
  //@}

  virtual double GetValue() const override;

protected:
  //@{
  /**
   * Default constructor and destructor.
   */
  vtkBinsAccumulator();
  virtual ~vtkBinsAccumulator() override = default;
  //@}

  /**
   * Bins where the data is stored
   */
  BinsPointer Bins;

  /**
   * Function pointer to tell what we accumulate in the bins.
   */
  std::function<double(double)> FunctionOfW;

  /**
   * Width of the Bins
   */
  double DiscretizationStep;

  /**
   * Value accumulated when adding data.
   */
  double Value;

  /**
   * Storage of function name.
   */
  static std::unordered_map<double (*)(double), std::string> FunctionName;

private:
  vtkBinsAccumulator(vtkBinsAccumulator&) = delete;
  void operator=(vtkBinsAccumulator&) = delete;
};

#endif
