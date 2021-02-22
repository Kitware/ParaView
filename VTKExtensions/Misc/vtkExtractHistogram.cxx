/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractHistogram.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGraph.h"
#include "vtkIOStream.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <map>
#include <string>
#include <vector>

struct vtkEHInternals
{
  vtkEHInternals()
    : FieldAssociation(-1)
  {
  }
  struct ArrayValuesType
  {
    // The total of the values per bin - the second vector
    // is for arrays with multiple components
    std::vector<std::vector<double> > TotalValues;
  };
  typedef std::map<std::string, ArrayValuesType> ArrayMapType;
  ArrayMapType ArrayValues;
  int FieldAssociation;
};

vtkStandardNewMacro(vtkExtractHistogram);
//-----------------------------------------------------------------------------
vtkExtractHistogram::vtkExtractHistogram()
  : Component(0)
  , BinCount(10)
{
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
  this->Internal = new vtkEHInternals;
  this->CalculateAverages = 0;
  this->CenterBinsAroundMinAndMax = false;
  this->UseCustomBinRanges = false;
  this->CustomBinRanges[0] = 0;
  this->CustomBinRanges[1] = 100;
}

//-----------------------------------------------------------------------------
vtkExtractHistogram::~vtkExtractHistogram()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkExtractHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Component: " << this->Component << "\n";
  os << indent << "BinCount: " << this->BinCount << "\n";
  os << indent << "CenterBinsAroundMinAndMax: " << this->CenterBinsAroundMinAndMax << "\n";
  os << indent << "UseCustomBinRanges: " << this->UseCustomBinRanges << "\n";
  os << indent << "CustomBinRanges: " << this->CustomBinRanges[0] << ", "
     << this->CustomBinRanges[1] << endl;
}

//-----------------------------------------------------------------------------
int vtkExtractHistogram::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkExtractHistogram::GetInputFieldAssociation()
{
  vtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  vtkInformation* inArrayInfo = inArrayVec->GetInformationObject(0);
  return inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
}

//-----------------------------------------------------------------------------
vtkFieldData* vtkExtractHistogram::GetInputFieldData(vtkDataObject* input)
{
  if (this->Internal->FieldAssociation < 0)
  {
    this->Internal->FieldAssociation = this->GetInputFieldAssociation();
  }

  switch (this->Internal->FieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    case vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS:
      return vtkDataSet::SafeDownCast(input)->GetPointData();
      break;
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      return vtkDataSet::SafeDownCast(input)->GetCellData();
      break;
    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      return input->GetFieldData();
      break;
    case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
      return vtkGraph::SafeDownCast(input)->GetVertexData();
      break;
    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
      return vtkGraph::SafeDownCast(input)->GetEdgeData();
      break;
    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      return vtkTable::SafeDownCast(input)->GetRowData();
      break;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
bool vtkExtractHistogram::GetInputArrayRange(vtkInformationVector** inputVector, double range[2])
{
  range[0] = VTK_DOUBLE_MAX;
  range[1] = VTK_DOUBLE_MIN;

  // obtain a pointer to the name of the vtkDataArray to bin up
  // and find the range of the data values within it
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* cdin = vtkCompositeDataSet::SafeDownCast(input);
  if (cdin)
  {
    // for composite datasets, visit each leaf data set and compute the total
    // range
    vtkCompositeDataIterator* cdit = cdin->NewIterator();
    cdit->InitTraversal();
    bool foundone = false;
    while (!cdit->IsDoneWithTraversal())
    {
      vtkDataObject* dObj = cdit->GetCurrentDataObject();
      vtkDataArray* data_array = this->GetInputArrayToProcess(0, dObj);
      if (data_array && this->Component >= 0 &&
        this->Component <= data_array->GetNumberOfComponents())
      {
        foundone = true;
        double tRange[2];
        if (this->Component == data_array->GetNumberOfComponents())
        {
          // magnitude
          data_array->GetRange(tRange, -1);
        }
        else
        {
          data_array->GetRange(tRange, this->Component);
        }
        range[0] = (tRange[0] < range[0]) ? tRange[0] : range[0];
        range[1] = (tRange[1] > range[1]) ? tRange[1] : range[1];
      }
      cdit->GoToNextItem();
    }
    cdit->Delete();

    if (!foundone)
    {
      return false;
    }
  }
  else
  {
    vtkDataArray* data_array = this->GetInputArrayToProcess(0, inputVector);
    if (!data_array)
    {
      return false;
    }
    // If the requested component is out-of-range for the input, we return an
    // empty dataset
    if (this->Component < 0 && this->Component > data_array->GetNumberOfComponents())
    {
      vtkWarningMacro("Requested component " << this->Component << " is not available.");
      return false;
    }
    if (this->Component == data_array->GetNumberOfComponents())
    {
      // magnitude
      data_array->GetRange(range, -1);
    }
    else
    {
      data_array->GetRange(range, this->Component);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkExtractHistogram::InitializeBinExtents(
  vtkInformationVector** inputVector, vtkDoubleArray* bin_extents, double& min, double& max)
{
  double range[2];
  range[0] = VTK_DOUBLE_MAX;
  range[1] = -VTK_DOUBLE_MAX;

  // Keeping the column name constant causes less issues in the GUI.
  bin_extents->SetName("bin_extents");

  if (this->UseCustomBinRanges)
  {
    range[0] = this->CustomBinRanges[0];
    range[1] = this->CustomBinRanges[1];
  }
  else if (!this->GetInputArrayRange(inputVector, range) || (range[0] > range[1]))
  {
    // We don't flag this as error since the array may just be missing for
    // current time-step (BUG #12290).
    vtkDebugMacro("Could not determine array range. "
                  "The chosen array or component may not be available or "
                  "has invalid range");
    return false;
  }

  // Calculate the extents of each bin, based on the range of values in the
  // input ...
  if (range[0] == range[1])
  {
    // Give it some width.
    range[1] = range[0] + 1;
  }

  min = range[0];
  max = range[1];
  this->FillBinExtents(bin_extents, min, max);
  return true;
}

//-----------------------------------------------------------------------------
void vtkExtractHistogram::FillBinExtents(vtkDoubleArray* bin_extents, double min, double max)
{
  // Calculate the extents of each bin, based on the range of values in the
  // input ...
  if (min == max)
  {
    // Give it some width.
    max = min + 1;
  }

  bin_extents->SetNumberOfComponents(1);
  bin_extents->SetNumberOfTuples(this->BinCount);
  double bin_delta =
    (max - min) / (this->CenterBinsAroundMinAndMax ? (this->BinCount - 1) : this->BinCount);
  double half_delta = bin_delta / 2.0;
  for (int i = 0; i < this->BinCount; ++i)
  {
    bin_extents->SetValue(
      i, min + (i * bin_delta) + (this->CenterBinsAroundMinAndMax ? 0. : half_delta));
  }
}

inline int vtkExtractHistogramClamp(int value, int min, int max)
{
  value = value < min ? min : value;
  value = value > max ? max : value;
  return value;
}

//-----------------------------------------------------------------------------
void vtkExtractHistogram::BinAnArray(
  vtkDataArray* data_array, vtkIntArray* bin_values, double min, double max, vtkFieldData* field)
{
  // If the requested component is out-of-range for the input,
  // the bin_values will be 0, so no need to do any actual counting.
  if (data_array == nullptr || this->Component < 0 ||
    this->Component > data_array->GetNumberOfComponents())
  {
    return;
  }

  int num_of_tuples = data_array->GetNumberOfTuples();
  double bin_delta =
    (max - min) / (this->CenterBinsAroundMinAndMax ? (this->BinCount - 1) : this->BinCount);
  double half_delta = bin_delta / 2.0;

  for (int i = 0; i != num_of_tuples; ++i)
  {
    if (i % 1000 == 0)
    {
      this->UpdateProgress(0.10 + 0.90 * i / num_of_tuples);
    }
    double value;
    // if component is equal to the number of components, then the magnitude was requested.
    if (this->Component == data_array->GetNumberOfComponents())
    {
      value = 0;
      double comp;
      for (int j = 0; j < data_array->GetNumberOfComponents(); ++j)
      {
        comp = data_array->GetComponent(i, j);
        value += comp * comp;
      }
      value = sqrt(value);
    }
    else
    {
      value = data_array->GetComponent(i, this->Component);
    }
    int index = static_cast<int>(
      (value - min + (this->CenterBinsAroundMinAndMax ? half_delta : 0.)) / bin_delta);

    // If the value is equal to max, include it in the last bin.
    index = ::vtkExtractHistogramClamp(index, 0, this->BinCount - 1);
    bin_values->SetValue(index, bin_values->GetValue(index) + 1);

    if (this->CalculateAverages)
    {
      // Get all other arrays, add their value to the bin
      // For each bin, we will need 2 values per array ->
      // total, num. elements
      // at the end, divide each total by num. elements
      int num_arrays = field->GetNumberOfArrays();
      for (int idx = 0; idx < num_arrays; idx++)
      {
        vtkDataArray* array = field->GetArray(idx);
        if (array && array != data_array && array->GetName())
        {
          vtkEHInternals::ArrayValuesType& arrayValues =
            this->Internal->ArrayValues[array->GetName()];
          arrayValues.TotalValues.resize(this->BinCount);
          int numComps = array->GetNumberOfComponents();
          arrayValues.TotalValues[index].resize(numComps);
          for (int comp = 0; comp < numComps; comp++)
          {
            arrayValues.TotalValues[index][comp] += array->GetComponent(i, comp);
          }
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
int vtkExtractHistogram::RequestData(vtkInformation* /*request*/,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Build an empty output grid in advance, so we can bail-out if we
  // encounter any problems
  vtkTable* const output_data = vtkTable::GetData(outputVector, 0);
  output_data->Initialize();

  if (this->UseCustomBinRanges && this->CustomBinRanges[1] < this->CustomBinRanges[0])
  {
    double min = this->CustomBinRanges[1];
    double max = this->CustomBinRanges[0];
    this->CustomBinRanges[0] = min;
    this->CustomBinRanges[1] = max;
    vtkWarningMacro("Custom bin range adjusted to keep min <= max value");
  }

  // These are the mid-points for each of the bins
  vtkSmartPointer<vtkDoubleArray> bin_extents = vtkSmartPointer<vtkDoubleArray>::New();
  bin_extents->SetNumberOfComponents(1);
  bin_extents->SetNumberOfTuples(this->BinCount);
  bin_extents->SetName("bin_extents");
  bin_extents->FillComponent(0, 0.0);

  // Insert values into bins ...
  vtkSmartPointer<vtkIntArray> bin_values = vtkSmartPointer<vtkIntArray>::New();
  bin_values->SetNumberOfComponents(1);
  bin_values->SetNumberOfTuples(this->BinCount);
  bin_values->SetName("bin_values");
  bin_values->FillComponent(0, 0.0);

  // Initializes the bin_extents array.
  double min, max;
  if (!this->InitializeBinExtents(inputVector, bin_extents, min, max))
  {
    this->Internal->ArrayValues.clear();
    return 1;
  }

  output_data->GetRowData()->AddArray(bin_extents);
  output_data->GetRowData()->AddArray(bin_values);

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* cdin = vtkCompositeDataSet::SafeDownCast(input);
  if (cdin)
  {
    // for composite datasets visit each leaf dataset and add in its counts
    vtkCompositeDataIterator* cdit = cdin->NewIterator();
    cdit->InitTraversal();
    while (!cdit->IsDoneWithTraversal())
    {
      vtkDataObject* dObj = cdit->GetCurrentDataObject();
      vtkDataArray* data_array = this->GetInputArrayToProcess(0, dObj);
      this->BinAnArray(data_array, bin_values, min, max, this->GetInputFieldData(dObj));
      cdit->GoToNextItem();
    }
    cdit->Delete();
  }
  else
  {
    vtkDataArray* data_array = this->GetInputArrayToProcess(0, inputVector);
    this->BinAnArray(data_array, bin_values, min, max, this->GetInputFieldData(input));
  }

  if (this->CalculateAverages)
  {
    vtkEHInternals::ArrayMapType::iterator iter = this->Internal->ArrayValues.begin();
    for (; iter != this->Internal->ArrayValues.end(); iter++)
    {
      vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
      std::string newname = iter->first + "_total";
      da->SetName(newname.c_str());
      vtkSmartPointer<vtkDoubleArray> aa = vtkSmartPointer<vtkDoubleArray>::New();
      std::string newname2 = iter->first + "_average";
      aa->SetName(newname2.c_str());
      int numComps = static_cast<int>(iter->second.TotalValues[0].size());
      da->SetNumberOfComponents(numComps);
      da->SetNumberOfTuples(this->BinCount);
      aa->SetNumberOfComponents(numComps);
      aa->SetNumberOfTuples(this->BinCount);
      for (vtkIdType i = 0; i < this->BinCount; i++)
      {
        for (int j = 0; j < numComps; j++)
        {
          if (iter->second.TotalValues[i].size() == (unsigned int)numComps)
          {
            da->SetValue(i * numComps + j, iter->second.TotalValues[i][j]);
            if (bin_values->GetValue(i))
            {
              aa->SetValue(
                i * numComps + j, iter->second.TotalValues[i][j] / bin_values->GetValue(i));
            }
            else
            {
              aa->SetValue(i * numComps + j, 0);
            }
          }
          else
          {
            da->SetValue(i * numComps + j, 0);
            aa->SetValue(i * numComps + j, 0);
          }
        }
      }
      output_data->GetRowData()->AddArray(da);
      output_data->GetRowData()->AddArray(aa);
    }

    this->Internal->ArrayValues.clear();
  }

  return 1;
}
