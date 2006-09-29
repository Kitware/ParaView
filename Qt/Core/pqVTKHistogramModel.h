
/// \file pqVTKHistogramModel.h
/// \date 9/27/2006

#ifndef _pqVTKHistogramModel_h
#define _pqVTKHistogramModel_h


#include "pqHistogramModel.h"

class pqVTKHistogramModelInternal;
class vtkRectilinearGrid;
class vtkDataObject;


class pqVTKHistogramModel : public pqHistogramModel
{
public:
  pqVTKHistogramModel(QObject *parent=0);
  virtual ~pqVTKHistogramModel();

  /// \name pqHistogramModel Methods
  //@{
  virtual int getNumberOfBins() const;
  virtual void getBinValue(int index, pqChartValue &bin) const;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const;

  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const;
  //@}

  /// Fetches the histogram data from the pipeline.
  void updateData(vtkRectilinearGrid *data);
  void updateData(vtkDataObject* data);

  void update();
  void forceUpdate();
private:
  pqVTKHistogramModelInternal *Internal; ///< Stores the data bounds.
  vtkRectilinearGrid *Data;              ///< A pointer to the data.
};

#endif

