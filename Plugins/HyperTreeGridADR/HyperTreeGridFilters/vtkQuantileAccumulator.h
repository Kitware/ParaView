/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuantileAccumulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkQuantileAccumulator
 * @brief   accumulates input data in a sorted array
 *
 * Accumulator for computing the median of the input data.
 * Inserting data is logarithmic in function of the input size,
 * while merging has a linear complexity.
 * Accessing the median from accumulated data has constant complexity
 *
 */

#ifndef vtkQuantileAccumulator_h
#define vtkQuantileAccumulator_h

#include "vtkAbstractAccumulator.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

#include <memory>
#include <vector>

class vtkDataObject;

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkQuantileAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkQuantileAccumulator* New();

  vtkTypeMacro(vtkQuantileAccumulator, vtkAbstractAccumulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using Superclass::Add;

  /**
   * Type of elements in list SortedList.
   */
  struct ListElement
  {
    /**
     * Constructor.
     */
    ListElement(double value, double weight);

    /**
     * Actual value accumulated
     */
    double Value;

    /**
     * Corresponding weight to Value.
     */
    double Weight;

    /**
     * Overriden operator< for sorting elements. Elements are sorted only regarding to the rank of
     * vtkQuantileAccumulator::ListElement::Value.
     */
    bool operator<(const ListElement&) const;
  };

  /**
   * Type of the sorted list.
   */
  typedef std::vector<ListElement> ListType;

  /**
   * Type of smart pointer on the sorted list.
   */
  typedef std::shared_ptr<ListType> ListPointer;

  //@{
  /**
   * Methods for adding data to the accumulator.
   */
  virtual void Add(vtkAbstractAccumulator* accumulator) override;
  virtual void Add(double value, double weight = 1.0) override;
  //@}

  /**
   * Set object into initial state
   */
  virtual void Initialize() override;

  /**
   * ShallowCopy implementation, both object then share the same SortedList.
   */
  virtual void ShallowCopy(vtkDataObject* accumulator) override;

  /**
   * DeepCopy implementation.
   */
  virtual void DeepCopy(vtkDataObject* accumulator) override;

  /**
   * Getter of internally stored sorted list of values and weights
   */
  const ListPointer& GetSortedList() const;

  /**
   * Returns true if the parameters of accumulator is the same as the ones of this
   */
  virtual bool HasSameParameters(vtkAbstractAccumulator* accumulator) const override;

  /**
   *
   */
  virtual double GetValue() const override;

  //@{
  /**
   * Set / Get on the Percentile to compute.
   */
  vtkGetMacro(Percentile, double);
  vtkSetMacro(Percentile, double);
  //@}

  /**
   * Getter for the index of the percentile in the sorted list.
   */
  vtkGetMacro(PercentileIdx, std::size_t);

  /**
   * Getter for the total weight accumulated.
   */
  vtkGetMacro(TotalWeight, double);

  /**
   * Getter for the cumulated weights of the values under the percentile.
   */
  vtkGetMacro(PercentileWeight, double);

protected:
  /**
   * Default constructor and destructor.
   */
  vtkQuantileAccumulator();
  virtual ~vtkQuantileAccumulator() override = default;

  /**
   * Index of the targetted value.
   */
  std::size_t PercentileIdx;

  /**
   * Percentile to compute.
   */
  double Percentile;

  /**
   * Accumulated weight though calls of vtkQuantileAccumulator::Add.
   */
  double TotalWeight;

  /**
   * Sum of all weights of value below percentile, including percentile.
   */
  double PercentileWeight;

  /**
   * Accumulated sorted list of values.
   */
  ListPointer SortedList;

private:
  vtkQuantileAccumulator(vtkQuantileAccumulator&) = delete;
  void operator=(vtkQuantileAccumulator&) = delete;
};

#endif
