#ifndef vtkEntropyArrayMeasurement_h
#define vtkEntropyArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class vtkEntropyArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkEntropyArrayMeasurement* New();

  vtkTypeMacro(vtkEntropyArrayMeasurement, vtkAbstractArrayMeasurement);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Returns the measure of input data.
   */
  virtual double Measure() const override;
  //@}

protected:
  //@{
  /**
   * Default constructors and destructors
   */
  vtkEntropyArrayMeasurement();
  virtual ~vtkEntropyArrayMeasurement() override = default;
  //@}

private:
  vtkEntropyArrayMeasurement(const vtkEntropyArrayMeasurement&) = delete;
  void operator=(const vtkEntropyArrayMeasurement&) = delete;
};

#endif
