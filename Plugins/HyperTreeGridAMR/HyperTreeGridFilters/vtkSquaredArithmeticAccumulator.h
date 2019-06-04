#ifndef vtkSquaredArithmeticAccumulator_h
#define vtkSquaredArithmeticAccumulator_h

#include "vtkAbstractAccumulator.h"

class vtkSquaredArithmeticAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkSquaredArithmeticAccumulator* New();

  vtkTypeMacro(vtkSquaredArithmeticAccumulator, vtkAbstractAccumulator);
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
  vtkSquaredArithmeticAccumulator();
  virtual ~vtkSquaredArithmeticAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated value
   */
  double Value;
  //@}

private:
  vtkSquaredArithmeticAccumulator(const vtkSquaredArithmeticAccumulator&) = delete;
  void operator=(const vtkSquaredArithmeticAccumulator&) = delete;
};

#endif
