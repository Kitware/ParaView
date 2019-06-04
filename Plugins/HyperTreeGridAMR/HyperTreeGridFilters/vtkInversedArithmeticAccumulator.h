#ifndef vtkInversedArithmeticAccumulator_h
#define vtkInversedArithmeticAccumulator_h

#include "vtkAbstractAccumulator.h"

class vtkInversedArithmeticAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkInversedArithmeticAccumulator* New();

  vtkTypeMacro(vtkInversedArithmeticAccumulator, vtkAbstractAccumulator);
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
  vtkInversedArithmeticAccumulator();
  virtual ~vtkInversedArithmeticAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated value
   */
  double Value;
  //@}

private:
  vtkInversedArithmeticAccumulator(const vtkInversedArithmeticAccumulator&) = delete;
  void operator=(const vtkInversedArithmeticAccumulator&) = delete;
};

#endif
