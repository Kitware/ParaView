#ifndef vtkGeometricMeanArrayMeasurement_h
#define vtkGeometricMeanArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class vtkGeometricMeanArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkGeometricMeanArrayMeasurement* New();

  vtkTypeMacro(vtkGeometricMeanArrayMeasurement, vtkAbstractArrayMeasurement);
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
  vtkGeometricMeanArrayMeasurement();
  virtual ~vtkGeometricMeanArrayMeasurement() override = default;
  //@}

private:
  vtkGeometricMeanArrayMeasurement(const vtkGeometricMeanArrayMeasurement&) = delete;
  void operator=(const vtkGeometricMeanArrayMeasurement&) = delete;
};

#endif
