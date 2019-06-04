#ifndef vtkHarmonicMeanArrayMeasurement_h
#define vtkHarmonicMeanArrayMeasurement_h

#include "vtkAbstractArrayMeasurement.h"

class vtkHarmonicMeanArrayMeasurement : public vtkAbstractArrayMeasurement
{
public:
  static vtkHarmonicMeanArrayMeasurement* New();

  vtkTypeMacro(vtkHarmonicMeanArrayMeasurement, vtkAbstractArrayMeasurement);
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
  vtkHarmonicMeanArrayMeasurement();
  virtual ~vtkHarmonicMeanArrayMeasurement() override = default;
  //@}

private:
  vtkHarmonicMeanArrayMeasurement(const vtkHarmonicMeanArrayMeasurement&) = delete;
  void operator=(const vtkHarmonicMeanArrayMeasurement&) = delete;
};

#endif
