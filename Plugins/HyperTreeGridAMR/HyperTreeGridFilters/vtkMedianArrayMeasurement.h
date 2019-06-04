#ifndef vtkMedianArrayMeasurement_h
#define vtkMedianArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class vtkMedianArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkMedianArrayMeasurement* New();

  vtkTypeMacro(vtkMedianArrayMeasurement, vtkAbstractArrayMeasurement);
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
  vtkMedianArrayMeasurement();
  virtual ~vtkMedianArrayMeasurement() override = default;
  //@}

private:
  vtkMedianArrayMeasurement(const vtkMedianArrayMeasurement&) = delete;
  void operator=(const vtkMedianArrayMeasurement&) = delete;
};

#endif
