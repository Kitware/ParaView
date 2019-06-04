#ifndef vtkAbstractArrayMeasurement_h
#define vtkAbstractArrayMeasurement_h

#include "vtkAbstractAccumulator.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObject.h"
#include "vtkSetGet.h"

#include <vector>

class vtkAbstractArrayMeasurement : public vtkObject
{
public:
  static vtkAbstractArrayMeasurement* New();

  vtkAbstractTypeMacro(vtkAbstractArrayMeasurement, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Methods used to add data to the accumulators.
   */
  virtual void Add(vtkDataArray* data);
  virtual void Add(double* data, vtkIdType numberOfComponents = 1);
  virtual void Add(vtkAbstractArrayMeasurement* arrayMeasurement);
  //@}

  //@{
  /**
   * Returns the measure of input data.
   */
  virtual double Measure() const = 0;
  //@}

  //@{
  /**
   * Set object into initial state.
   */
  virtual void Initialize();
  //@}

  //@{
  /**
   * Accessor for the number of values already fed for the measurement
   */
  vtkGetMacro(NumberOfAccumulatedData, vtkIdType);

  //@{
  /**
   * Number of accumulators needed for measurement
   */
  virtual std::size_t GetNumberOfAccumulators() const { return this->Accumulators.size(); }
  //@}

  //@{
  /**
   * Accessor for accumulators
   */
  virtual std::vector<vtkAbstractAccumulator*> GetAccumulators() const
  {
    return this->Accumulators;
  }
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkAbstractArrayMeasurement();
  virtual ~vtkAbstractArrayMeasurement() override;
  //@}

  //@{
  /**
   * Accumulators used to accumulate the input data.
   */
  std::vector<vtkAbstractAccumulator*> Accumulators;
  //@}

  //@{
  /**
   * Amount of data already fed to accumulators.
   */
  vtkIdType NumberOfAccumulatedData;

private:
  vtkAbstractArrayMeasurement(const vtkAbstractArrayMeasurement&) = delete;
  void operator=(const vtkAbstractArrayMeasurement&) = delete;
};

#endif
