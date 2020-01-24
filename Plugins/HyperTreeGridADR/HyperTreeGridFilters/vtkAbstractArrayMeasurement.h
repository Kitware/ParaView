/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractArrayMeasurement.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkAbstractArrayMeasurement
 * @brief   measures quantities on arrays of values
 *
 * Given an array of data, it computes the wanted quantity over the array.
 * Data can be fed using the whole array as input, or value by value,
 * or by merging two vtkAbstractArrayMeasurement*.
 * The complexity depends on the array of vtkAbstractAccumulator*
 * used to compute the measurement. Given n inputs, if f(n) is the
 * worst complexity given used accumulators, the complexity of inserting
 * data is O(n f(n)), and the complexity for measuring already
 * inserted data is O(1), unless said otherwise.
 * Merging complexity depends on the vtkAbstractAccumulator* used.
 *
 * @note The macro vtkArrayMeasurementMacro(thisClass) assumes that each subclass
 * of vtkAbstractArrayMeasurement implements the following static methods and attributes:
 * \code{.cpp}
 * static constexpr vtkIdType MinimumNumberOfAccumulatedData;
 * static constexpr vtkIdType NumberOfAccumulators;
 * static bool IsMeasurable(vtkIdType, double);
 * static bool Measure(vtkAbstractAccumulator**, vtkIdType, double, double&);
 * static std::vector<vtkAbstractAccumulator*> NewAccumulators();
 * \endcode
 *
 * This macro implements the override version of each corresponding pure virtual method of this
 * class.
 *
 */

#ifndef vtkAbstractArrayMeasurement_h
#define vtkAbstractArrayMeasurement_h

#include "vtkDataObject.h"
#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

#include <vector>

// Macro to implement automatically the pure virtual methods in the subclasses.
// It assumes a few static methods / attributs are implemented.
#define vtkArrayMeasurementMacro(thisClass)                                                        \
  bool thisClass::CanMeasure(vtkIdType numberOfAccumulatedData, double totalWeight) const          \
  {                                                                                                \
    return thisClass::IsMeasurable(numberOfAccumulatedData, totalWeight);                          \
  }                                                                                                \
                                                                                                   \
  std::vector<vtkAbstractAccumulator*> thisClass::NewAccumulatorInstances() const                  \
  {                                                                                                \
    return thisClass::NewAccumulators();                                                           \
  }                                                                                                \
                                                                                                   \
  vtkIdType thisClass::GetMinimumNumberOfAccumulatedData() const                                   \
  {                                                                                                \
    return thisClass::MinimumNumberOfAccumulatedData;                                              \
  }                                                                                                \
                                                                                                   \
  vtkIdType thisClass::GetNumberOfAccumulators() const                                             \
  {                                                                                                \
    return thisClass::NumberOfAccumulatedData;                                                     \
  }                                                                                                \
  bool thisClass::IsMeasurable(vtkIdType numberOfAccumulatedData, double totalWeight)              \
  {                                                                                                \
    return numberOfAccumulatedData >= thisClass::MinimumNumberOfAccumulatedData &&                 \
      totalWeight != 0.0;                                                                          \
  }

class vtkAbstractAccumulator;
class vtkDataArray;
class vtkDoubleArray;

class VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkAbstractArrayMeasurement : public vtkDataObject
{
public:
  vtkAbstractTypeMacro(vtkAbstractArrayMeasurement, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkAbstractArrayMeasurement* New();

  /**
   * Method used to add data to the accumulators.
   *
   * @param data is an array of vectors to accumulate.
   * @param weights are the weights associated to each point of data. It does not have to be set.
   * to use this method
   */
  virtual void Add(vtkDataArray* data, vtkDoubleArray* weights = nullptr);

  /**
   * Method used to add data to the accumulators.
   *
   * @param data is one vector to accumulate. The array should be of size numberOfComponents.
   * @param numberOfComponents specifies the dimension of the input vectors.
   * @param weight is the weight associated to the input vector.
   * to use this method
   */
  virtual void Add(double* data, vtkIdType numberOfComponents = 1, double weight = 1.0);

  /**
   * Method used to add the accumulated data of another vtkAbstractArrayMeasurement to this
   * instance.
   *
   * @param arrayMeasurement is another vtkAbstractArrayMeasurement to add, which MUST be of the
   * same dynamic type as this instance, with the same parameters.
   */
  virtual void Add(vtkAbstractArrayMeasurement* arrayMeasurement);

  /**
   * Notifies if the accumulated data is suitable for measuring.
   * The second implementation aims to be able to dynamically tell whether accumulated data is fit
   * for measurement.
   *
   * @param numberOfAccumulatedData is the number of times one element was added to the accumulators
   * to measure.
   * @param totalWeight is the accumulated weight. If no weights were set, it should be equal to
   * numberOfAccumulatedData.
   * @return true if the accumulated data can be measured.
   */
  bool CanMeasure() const;
  virtual bool CanMeasure(vtkIdType numberOfAccumulatedData, double totalWeight) const = 0;
  //@}

  /**
   * Measures the accumulated data with the corresponding method.
   * The second implementation aims to be able to dynamically measure accumulated data with outer
   * accumulators.
   *
   * @param accumulators is an array of accumulators necessary for measuring.
   * @param numberOfAccumulatedData
   * @return the value of vtkAbstractArrayMeasurement::CanMeasure
   */
  bool Measure(double& value);
  virtual bool Measure(vtkAbstractAccumulator** accumulators, vtkIdType numberOfAccumulatedData,
    double totalWeight, double& value) = 0;
  //@}

  /**
   * Dynamically instanciates accumulators needed for measuring.
   *
   * @return an std::vector of accumulators of size GetNumberOfAccumulators()
   * @warning The order of those accumulators is crucial, since there is no further check
   * when measuring on whether the input accumulators are in order or not. We assume
   * that when one measures accumulators, they are in the same order as when this class
   * instantiates them.
   */
  virtual std::vector<vtkAbstractAccumulator*> NewAccumulatorInstances() const = 0;

  /**
   * Accessor for the minimum number of accumulated data necessary for computing the measure.
   *
   * @note Every subclasses should have a
   * \code{.cpp}static constexpr vtkIdtype MinimumNumberOfAccumulatedData\endcode.
   */
  virtual vtkIdType GetMinimumNumberOfAccumulatedData() const = 0;

  /**
   * Set object into initial state.
   */
  void Initialize() override;

  /**
   * Accessor for the number of values already fed for the measurement
   */
  vtkGetMacro(NumberOfAccumulatedData, vtkIdType);

  /**
   * Accessor for the total accumulated weight. If no weight was fed, it equals
   * NumberOfAccumulatedData.
   */
  vtkGetMacro(TotalWeight, double);

  /**
   * Accessor for the number of accumulators needed to measure.
   *
   * @note The macro vtkArrayMeasurementMacro implementing this method in subclasses assumes
   * that the subclasses have an instance of
   * \code{.cpp}static constexpr vtkIdType NumberOfAccumulators\endcode
   */
  virtual vtkIdType GetNumberOfAccumulators() const = 0;

  //@{
  /**
   * Accessor for inner accumulators.
   */
  virtual const std::vector<vtkAbstractAccumulator*>& GetAccumulators() const;
  virtual std::vector<vtkAbstractAccumulator*>& GetAccumulators();
  //@}

  /**
   * ShallowCopy implementation.
   */
  virtual void ShallowCopy(vtkDataObject* o) override;

  /**
   * DeepCopy implementation.
   */
  virtual void DeepCopy(vtkDataObject* o) override;

protected:
  //@{
  /**
   * Default constructor and destructor
   *
   * @warning Accumulators are not allocated in the constructor.
   */
  vtkAbstractArrayMeasurement();
  virtual ~vtkAbstractArrayMeasurement() override;
  //@}

  /**
   * Accumulators used to accumulate the input data.
   */
  std::vector<vtkAbstractAccumulator*> Accumulators;

  /**
   * Amount of data already fed to accumulators.
   */
  vtkIdType NumberOfAccumulatedData;

  /**
   * Accumulated weight
   */
  double TotalWeight;

private:
  vtkAbstractArrayMeasurement(vtkAbstractArrayMeasurement&) = delete;
  void operator=(vtkAbstractArrayMeasurement&) = delete;
};

#endif
