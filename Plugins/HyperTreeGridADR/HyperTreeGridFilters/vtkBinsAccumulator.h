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
 * The user should set the function pointer Functor, which is by default the identity.
 * A collection of usable functions can be find in header vtkFunctionOfXList.h.
 * Functor is used as follows: the accumulated value equals sum_i Functor(w_i).
 * where w_i is the accumulated weight at bin i.
 *
 * One can use this accumulator to compute the entropy by setting Functor = VTK_FUNC_XLOGX
 * if the header vtkFunctionOfXList.h is included. Normalizing the result shall be performed.
 */

#ifndef vtkBinsAccumulator_h
#define vtkBinsAccumulator_h

#include "vtkAbstractAccumulator.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

#include <memory>
#include <string>
#include <unordered_map>

template <typename FunctorT>
class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkBinsAccumulator : public vtkAbstractAccumulator
{
public:
  typedef std::unordered_map<long long, double> BinsType;
  typedef std::shared_ptr<BinsType> BinsPointer;
  typedef FunctorT FunctorType;

  static vtkBinsAccumulator* New();

  vtkTemplateTypeMacro(vtkBinsAccumulator, vtkAbstractAccumulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using Superclass::Add;

  //@{
  /**
   * Methods for adding data to the accumulator
   */
  void Add(vtkAbstractAccumulator* accumulator) override;
  void Add(double value, double weight = 1.0) override;
  //@}

  /**
   * Clears the bins.
   */
  void Initialize() override;

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
  void DeepCopy(vtkDataObject* accumulator) override;

  /**
   * DeepCopy implementation.
   */
  void ShallowCopy(vtkDataObject* accumulator) override;

  /**
   * Returns true if the parameters of accumulator is the same as the ones of this
   */
  bool HasSameParameters(vtkAbstractAccumulator* accumulator) const override;

  //@{
  /**
   * Accessor/mutator on the function pointer specifying which quantity should be computed on the
   * bins. Bins are filled with the accumulated weight of the corresponding value range. The
   * accumulated value is sum_i Functor(w_i), where w_i is the weight at bin i.
   */
  const FunctorT& GetFunctor() const;
  void SetFunctor(const FunctorT&);
  //@}

  double GetValue() const override;

protected:
  //@{
  /**
   * Default constructor and destructor.
   */
  vtkBinsAccumulator();
  ~vtkBinsAccumulator() override = default;
  //@}

  /**
   * Bins where the data is stored
   */
  BinsPointer Bins;

  /**
   * Function pointer to tell what we accumulate in the bins.
   */
  FunctorType Functor;

  /**
   * Width of the Bins
   */
  double DiscretizationStep;

  /**
   * Value accumulated when adding data.
   */
  double Value;

private:
  vtkBinsAccumulator(vtkBinsAccumulator<FunctorT>&) = delete;
  void operator=(vtkBinsAccumulator<FunctorT>&) = delete;
};

#include "vtkBinsAccumulator.txx"

#endif
