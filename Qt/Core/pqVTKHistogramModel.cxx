
#include "pqVTKHistogramModel.h"

#include "pqChartCoordinate.h"
#include <QtDebug>

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnsignedLongArray.h"


class pqVTKHistogramModelInternal
{
public:
  pqVTKHistogramModelInternal();
  ~pqVTKHistogramModelInternal() {}

  pqChartCoordinate Minimum;
  pqChartCoordinate Maximum;
};


//----------------------------------------------------------------------------
pqVTKHistogramModelInternal::pqVTKHistogramModelInternal()
  : Minimum(), Maximum()
{
}


//----------------------------------------------------------------------------
pqVTKHistogramModel::pqVTKHistogramModel(QObject *parentObject)
  : pqHistogramModel(parentObject)
{
  this->Internal = new pqVTKHistogramModelInternal();
  this->Data = 0;
}

pqVTKHistogramModel::~pqVTKHistogramModel()
{
  delete this->Internal;
  if(this->Data)
    {
    this->Data->Delete();
    }
}

int pqVTKHistogramModel::getNumberOfBins() const
{
  vtkUnsignedLongArray *const values = vtkUnsignedLongArray::SafeDownCast(
      this->Data->GetCellData()->GetArray("bin_values"));
  if(values && values->GetNumberOfComponents() == 1)
    {
    return values->GetNumberOfTuples();
    }

  return 0;
}

void pqVTKHistogramModel::getBinValue(int index, pqChartValue &bin) const
{
  vtkUnsignedLongArray *const values = vtkUnsignedLongArray::SafeDownCast(
      this->Data->GetCellData()->GetArray("bin_values"));
  if(values && values->GetNumberOfComponents() == 1 && index >= 0 &&
      index < values->GetNumberOfTuples())
    {
    bin = static_cast<double>(values->GetValue(index));
    }
}

void pqVTKHistogramModel::getRangeX(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->Minimum.X;
  max = this->Internal->Maximum.X;
}

void pqVTKHistogramModel::getRangeY(pqChartValue &min, pqChartValue &max) const
{
  min = this->Internal->Minimum.Y;
  max = this->Internal->Maximum.Y;
}

void pqVTKHistogramModel::updateData(vtkRectilinearGrid *data)
{
  // Release the reference to the old data if there is any.
  if(this->Data)
    {
    this->Data->Delete();
    this->Data = 0;
    this->Internal->Minimum.X = (int)0;
    this->Internal->Maximum.X = (int)0;
    this->Internal->Minimum.Y = (int)0;
    this->Internal->Maximum.Y = (int)0;
    }

  // Keep a reference to the new data.
  this->Data = data;
  this->Data->Register(0);

  // Get the overall range for the histogram. The bin ranges are
  // stored in the x coordinate array.
  vtkDoubleArray *const extents = vtkDoubleArray::SafeDownCast(
      this->Data->GetXCoordinates());
  if(!extents || extents->GetNumberOfComponents() != 1)
    {
    qWarning("Unrecognized histogram extent data. The histogram model expects "
        "a double array of tuples with one component.");
    }
  else if(extents->GetNumberOfTuples() < 2)
    {
    qWarning("The histogram range must have at least two values.");
    }
  else
    {
    this->Internal->Minimum.X = extents->GetValue(0);
    this->Internal->Maximum.X = extents->GetValue(
        extents->GetNumberOfTuples() - 1);

    // Search through the bin values to find the y-axis range.
    vtkUnsignedLongArray *const values = vtkUnsignedLongArray::SafeDownCast(
        this->Data->GetCellData()->GetArray("bin_values"));
    if(!values || values->GetNumberOfComponents() != 1)
      {
      qWarning("Unrecognized histogram data. The histogram model expects "
          "an unsigned long array of tuples with one component.");
      }
    else
      {
      unsigned long min = 0;
      unsigned long max = 0;
      unsigned long value = 0;
      for(int i = 0; i != values->GetNumberOfTuples(); ++i)
        {
        value = values->GetValue(i);
        if(i == 0)
          {
          min = value;
          max = value;
          }
        else if(value < min)
          {
          min = value;
          }
        else if(value > max)
          {
          max = value;
          }
        }

      this->Internal->Minimum.Y = static_cast<double>(min);
      this->Internal->Maximum.Y = static_cast<double>(max);
      }
    }

  // Notify the chart of the change.
  this->resetBinValues();
}


