#ifndef vtkAbstractAccumulator_h
#define vtkAbstractAccumulator_h

#include "vtkDataArray.h"
#include "vtkObject.h"

#include <functional>

class vtkAbstractAccumulator : public vtkObject
{
public:
  static vtkAbstractAccumulator* New();

  vtkAbstractTypeMacro(vtkAbstractAccumulator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Methods for adding data to the accumulator.
   */
  virtual void Add(vtkDataArray* data);
  virtual void Add(const double* data, vtkIdType numberOfElements = 1);
  virtual void Add(vtkAbstractAccumulator* accumulator) = 0;
  virtual void Add(double value) = 0;
  //@}

  //@{
  /**
   * Accessor to the accumulated value.
   */
  virtual double GetValue() const = 0;
  //@}

  //@{
  /**
   * Set object into initial state
   */
  virtual void Initialize() = 0;
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkAbstractAccumulator();
  virtual ~vtkAbstractAccumulator() override = default;
  //@}

  //@{
  /**
   * Lambda expression converting vectors to scalars. Default function is regular L2 norm.
   */
  std::function<double(const double*, vtkIdType)> ConvertVectorToScalar;
  //@}

private:
  vtkAbstractAccumulator(const vtkAbstractAccumulator&) = delete;
  void operator=(const vtkAbstractAccumulator&) = delete;
};

#endif
