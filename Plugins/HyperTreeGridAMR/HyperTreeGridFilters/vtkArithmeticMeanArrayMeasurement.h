#ifndef vtkArithmeticMeanArrayMeasurement_h
#define vtkArithmeticMeanArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class vtkArithmeticMeanArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkArithmeticMeanArrayMeasurement* New();

  vtkTypeMacro(vtkArithmeticMeanArrayMeasurement, vtkAbstractArrayMeasurement);
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
  vtkArithmeticMeanArrayMeasurement();
  virtual ~vtkArithmeticMeanArrayMeasurement() override = default;
  //@}

private:
  vtkArithmeticMeanArrayMeasurement(const vtkArithmeticMeanArrayMeasurement&) = delete;
  void operator=(const vtkArithmeticMeanArrayMeasurement&) = delete;
};

#endif
