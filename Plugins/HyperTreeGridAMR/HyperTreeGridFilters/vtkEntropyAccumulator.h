#ifndef vtkEntropyAccumulator_h
#define vtkEntropyAccumulator_h

#include "vtkAbstractAccumulator.h"

class vtkEntropyAccumulator : public vtkAbstractAccumulator
{
public:
  static vtkEntropyAccumulator* New();

  vtkTypeMacro(vtkEntropyAccumulator, vtkAbstractAccumulator);
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
  vtkEntropyAccumulator();
  virtual ~vtkEntropyAccumulator() override = default;
  //@}

  //@{
  /**
   * Accumulated value
   */
  double Value;
  //@}

private:
  vtkEntropyAccumulator(const vtkEntropyAccumulator&) = delete;
  void operator=(const vtkEntropyAccumulator&) = delete;
};

#endif
