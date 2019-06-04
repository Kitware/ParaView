#ifndef vtkGeometricAccumulator_h
#define vtkGeometricAccumulator_h

#include "vtkAbstractAccumulator.h"

class vtkGeometricAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkGeometricAccumulator* New();

  vtkTypeMacro(vtkGeometricAccumulator, vtkAbstractAccumulator);
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
  virtual double GetValue() const override { return this->Value; }
  //@}

  //@{
  /**
   * Set object into initial state
   */
  virtual void Initialize() override;
  //@}

protected:
  //@{
  /**
   * Default constructor and destructor.
   */
  vtkGeometricAccumulator();
  virtual ~vtkGeometricAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated value
   */
  double Value;
  //@}

private:
  vtkGeometricAccumulator(const vtkGeometricAccumulator&) = delete;
  void operator=(const vtkGeometricAccumulator&) = delete;
};

#endif
