#ifndef vtkArithmeticAccumulator_h
#define vtkArithmeticAccumulator_h

#include "vtkAbstractAccumulator.h"

class vtkArithmeticAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkArithmeticAccumulator* New();

  vtkTypeMacro(vtkArithmeticAccumulator, vtkAbstractAccumulator);
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
  vtkArithmeticAccumulator();
  virtual ~vtkArithmeticAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated value
   */
  double Value;
  //@}

private:
  vtkArithmeticAccumulator(const vtkArithmeticAccumulator&) = delete;
  void operator=(const vtkArithmeticAccumulator&) = delete;
};

#endif
