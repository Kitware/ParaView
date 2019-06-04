#ifndef vtkStandardDeviationArrayMeasurement_h
#define vtkStandardDeviationArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class vtkStandardDeviationArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkStandardDeviationArrayMeasurement* New();

  vtkTypeMacro(vtkStandardDeviationArrayMeasurement, vtkAbstractArrayMeasurement);
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
  vtkStandardDeviationArrayMeasurement();
  virtual ~vtkStandardDeviationArrayMeasurement() override = default;
  //@}

private:
  vtkStandardDeviationArrayMeasurement(const vtkStandardDeviationArrayMeasurement&) = delete;
  void operator=(const vtkStandardDeviationArrayMeasurement&) = delete;
};

#endif
