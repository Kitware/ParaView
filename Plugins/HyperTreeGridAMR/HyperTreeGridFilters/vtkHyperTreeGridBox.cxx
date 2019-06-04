#include "vtkHyperTreeGridBox.h"

#include "vtkDemandDrivenPipeline.h"
#include <vtkBitArray.h>
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkHyperTree.h>
#include <vtkHyperTreeGrid.h>
#include <vtkHyperTreeGridNonOrientedCursor.h>
#include <vtkHyperTreeGridNonOrientedGeometryCursor.h>
#include <vtkIdList.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include "vtkPVBox.h"
#include "vtkPVExtractVOI.h"
#include "vtkPVMetaClipDataSet.h"
#include <vtkBox.h>
#include <vtkDescriptiveStatistics.h>
#include <vtkExtractGeometry.h>
#include <vtkExtractGrid.h>
#include <vtkExtractRectilinearGrid.h>
#include <vtkExtractUnstructuredGrid.h>
#include <vtkExtractVOI.h>

#include <vtkPointDataToCellData.h>

#include <vtkXMLUnstructuredGridWriter.h>

#include <cmath>
#include <cstring>
#include <iostream>

vtkStandardNewMacro(vtkHyperTreeGridBox);

//----------------------------------------------------------------------------
vtkHyperTreeGridBox::vtkHyperTreeGridBox()
  : PointDataToCellDataConverter(vtkPointDataToCellData::New())
  , BranchFactor(2)
  , MaxTreeDepth(1)
  , UseMax(false)
  , UseMin(false)
  , UseEntropy(false)
  , UseRange(false)
  , MaskUsed(false)
  , InterceptUsed(false)
{
  // This a source: no input ports
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->ValueFunction = std::bind(&vtkHyperTreeGridBox::Mean, this, std::placeholders::_1);
  this->MaskFunction =
    std::bind(&vtkHyperTreeGridBox::MaskImplementation, this, std::placeholders::_1);
  this->MetricFunction =
    std::bind(&vtkHyperTreeGridBox::MetricImplementation, this, std::placeholders::_1);
}

//----------------------------------------------------------------------------
vtkHyperTreeGridBox::~vtkHyperTreeGridBox() = default;

//----------------------------------------------------------------------------
void vtkHyperTreeGridBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHyperTreeGridBox::GetOutput()
{
  return this->GetOutputDataObject(0);
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridBox::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridBox::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->GetNumberOfInputPorts() == 0 || this->GetNumberOfOutputPorts() == 0)
  {
    return 1;
  }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject* output = info->Get(vtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA("vtkHyperTreeGrid"))
      {
        vtkDataObject* newOutput = vtkHyperTreeGrid::New();
        info->Set(vtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridBox::FillInputPortInformation(int, vtkInformation* info)
{
  // This filter uses the vtkDataSet cell traversal methods so it
  // suppors any data set type as input.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

int vtkHyperTreeGridBox::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

int vtkHyperTreeGridBox::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the information objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // We cannot give the exact number of levels of the hypertrees
  // because it is not generated yet and this process depends on the recursion formula.
  // Just send an upper limit instead.
  outInfo->Set(vtkHyperTreeGrid::LEVELS(), this->MaxTreeDepth);
  outInfo->Set(vtkHyperTreeGrid::DIMENSION(), 3);
  outInfo->Set(vtkHyperTreeGrid::ORIENTATION(), 0);

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridBox::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->UpdateProgress(0.0);

  // Get input and output data.
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);

  // We are now executing this filter.
  vtkDebugMacro("Convert to HTG");

  // Skip execution if there is no input geometry.
  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numCells < 1 || numPts < 1)
  {
    vtkDebugMacro("No data to convert!");
    return 1;
  }

  output->Initialize();
  output->SetBranchFactor(this->BranchFactor);

  double* bounds = input->GetBounds();
  this->InputBounds = bounds;

  {
    vtkNew<vtkDoubleArray> coords;
    coords->SetNumberOfValues(this->Dimensions[0]);
    double step =
      this->Dimensions[0] > 1 ? (bounds[1] - bounds[0]) / (this->Dimensions[0] - 1) : 0.0;
    for (size_t i = 0; i < this->Dimensions[0]; ++i)
    {
      coords->SetValue(i, bounds[0] + step * i);
    }
    output->SetXCoordinates(coords);
  }

  {
    vtkNew<vtkDoubleArray> coords;
    coords->SetNumberOfValues(this->Dimensions[1]);
    double step =
      this->Dimensions[1] > 1 ? (bounds[3] - bounds[2]) / (this->Dimensions[1] - 1) : 0.0;
    for (size_t i = 0; i < this->Dimensions[1]; ++i)
    {
      coords->SetValue(i, bounds[2] + step * i);
    }
    output->SetYCoordinates(coords);
  }

  {
    vtkNew<vtkDoubleArray> coords;
    coords->SetNumberOfValues(this->Dimensions[2]);
    double step =
      this->Dimensions[2] > 1 ? (bounds[5] - bounds[4]) / (this->Dimensions[2] - 1) : 0.0;
    for (size_t i = 0; i < this->Dimensions[2]; ++i)
    {
      coords->SetValue(i, bounds[4] + step * i);
    }
    output->SetZCoordinates(coords);
  }

  output->SetDimensions(this->Dimensions);

  this->Input = input;

  vtkPVMetaClipDataSet* extract = vtkPVMetaClipDataSet::New();
  extract->SetInputDataObject(this->Input);
  extract->ExactBoxClipOn();
  extract->SetInsideOut(1);
  this->Extractor.push_back(extract);

  this->Mask = vtkBitArray::New();
  this->Mask->SetNumberOfValues(0);
  this->MaskUsed = false;

  this->Intercepts = vtkDoubleArray::New();
  this->Intercepts->SetName("Intercepts");
  this->Intercepts->SetNumberOfComponents(3);
  this->Normals = vtkDoubleArray::New();
  this->Normals->SetName("Normals");
  this->Normals->SetNumberOfComponents(3);
  this->InterceptUsed = false;

  vtkInformation* info = this->GetInputArrayInformation(0);
  int key = info->Get(vtkDataObject::FIELD_ARRAY_TYPE());
  vtkSmartPointer<vtkDataArray> data = this->GetDataObject(input, key);
  vtkSmartPointer<vtkDataArray> newData = data->NewInstance();
  newData->SetName(data->GetName());
  newData->SetNumberOfValues(0);
  output->GetPointData()->AddArray(newData);

  this->Output = output;
  this->MaskUsed = false;
  this->InterceptUsed = false;

  if (!this->GenerateTrees(nullptr, outputDO))
  {
    return 0;
  }

  // only add the mask array, if it contains non zero erntries
  if (this->MaskUsed)
  {
    output->SetMask(this->Mask);
  }
  // only add the intercept array, if it is used
  if (this->InterceptUsed)
  {
    output->GetPointData()->AddArray(this->Intercepts);
    output->GetPointData()->AddArray(this->Normals);
    output->SetInterfaceInterceptsName("Intercepts");
    output->SetInterfaceNormalsName("Normals");
    output->SetHasInterface(1);
  }

  // Avoid keeping extra memory around.
  output->Squeeze();

  this->UpdateProgress(1.0);

  return 1;
}

int vtkHyperTreeGridBox::GenerateTrees(vtkHyperTreeGrid*, vtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Iterate over all hyper trees
  vtkNew<vtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  size_t offsetIndex = 0;
  vtkIdType treeId;
  vtkIdType numTrees = output->GetMaxNumberOfTrees();
  double progress = 0;

  for (size_t i = 0; i < this->Dimensions[0] - 1 || (this->Dimensions[0] == 1 && i < 1); ++i)
  {
    for (size_t j = 0; j < this->Dimensions[1] - 1 || (this->Dimensions[1] == 1 && j < 1); ++j)
    {
      for (size_t k = 0; k < this->Dimensions[2] - 1 || (this->Dimensions[2] == 1 && k < 1); ++k)
      {
        output->GetIndexFromLevelZeroCoordinates(treeId, i, j, k);
        output->InitializeNonOrientedGeometryCursor(cursor, treeId, true);

        cursor->SetGlobalIndexStart(offsetIndex);
        this->SubdivideLeavesTillLevel(cursor);
        offsetIndex += cursor->GetTree()->GetNumberOfVertices();

        this->UpdateProgress(progress++ / numTrees);
      }
    }
  }

  return 1;
}

void vtkHyperTreeGridBox::SubdivideLeavesTillLevel(
  vtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  double bounds[6];
  cursor->GetBounds(bounds);
  vtkDataSet* dataSet = this->GetSubdividedData(bounds, cursor->GetLevel());

  vtkIdType idx = cursor->GetGlobalNodeIndex();

  int mask = 0;
  if (this->Input->GetDataObjectType() != VTK_IMAGE_DATA)
  {
    mask = this->MaskFunction(dataSet);
    this->Mask->InsertTuple1(idx, mask);
    this->MaskUsed |= mask == 1;
  }

  vtkDataArray* data = this->GetInputArrayToProcess(0, dataSet);
  if (!data)
  {
    return;
  }
  double value = this->ValueFunction(data);
  this->Output->GetPointData()->GetArray(0)->InsertTuple1(idx, value);

  if (!dataSet->GetNumberOfPoints())
  {
    return;
  }

  if (mask == 1)
  {
    return;
  }

  if (cursor->GetLevel() >= this->MaxTreeDepth)
  {
    return;
  }

  //  if (this->Input->GetDataObjectType() == VTK_IMAGE_DATA ||
  //  this->IsNodeWithinGeometry(dataSet, bounds))
  {
    if (this->MetricFunction(this->GetInputArrayToProcess(0, dataSet)))
    {
      return;
    }
  }

  cursor->SubdivideLeaf();

  unsigned char numChildren = cursor->GetNumberOfChildren();
  for (size_t childId = 0; childId < numChildren; ++childId)
  {
    cursor->ToChild(childId);
    this->SubdivideLeavesTillLevel(cursor);
    cursor->ToParent();
  }
}

bool vtkHyperTreeGridBox::IsNodeWithinGeometry(vtkDataSet* dataSet, double bounds[6])
{
  if (!dataSet->GetNumberOfPoints())
  {
    return false;
  }
  double minDistance = 1e-6;
  // search for closest point and measure distance
  {
    double queryPoint[3] = { bounds[0], bounds[2], bounds[4] };
    double* point = dataSet->GetPoint(dataSet->FindPoint(queryPoint));
    double distance[3] = { queryPoint[0] - point[0], queryPoint[1] - point[1],
      queryPoint[2] - point[2] };
    if (sqrt(distance[0] * distance[0] + distance[1] * distance[1] + distance[2] * distance[2]) >=
      minDistance)
      return false;
  }
  {
    double queryPoint[3] = { bounds[0], bounds[2], bounds[5] };
    double* point = dataSet->GetPoint(dataSet->FindPoint(queryPoint));
    double distance[3] = { queryPoint[0] - point[0], queryPoint[1] - point[1],
      queryPoint[2] - point[2] };
    if (sqrt(distance[0] * distance[0] + distance[1] * distance[1] + distance[2] * distance[2]) >=
      minDistance)
      return false;
  }
  {
    double queryPoint[3] = { bounds[0], bounds[3], bounds[4] };
    double* point = dataSet->GetPoint(dataSet->FindPoint(queryPoint));
    double distance[3] = { queryPoint[0] - point[0], queryPoint[1] - point[1],
      queryPoint[2] - point[2] };
    if (sqrt(distance[0] * distance[0] + distance[1] * distance[1] + distance[2] * distance[2]) >=
      minDistance)
      return false;
  }
  {
    double queryPoint[3] = { bounds[0], bounds[3], bounds[5] };
    double* point = dataSet->GetPoint(dataSet->FindPoint(queryPoint));
    double distance[3] = { queryPoint[0] - point[0], queryPoint[1] - point[1],
      queryPoint[2] - point[2] };
    if (sqrt(distance[0] * distance[0] + distance[1] * distance[1] + distance[2] * distance[2]) >=
      minDistance)
      return false;
  }
  {
    double queryPoint[3] = { bounds[1], bounds[2], bounds[4] };
    double* point = dataSet->GetPoint(dataSet->FindPoint(queryPoint));
    double distance[3] = { queryPoint[0] - point[0], queryPoint[1] - point[1],
      queryPoint[2] - point[2] };
    if (sqrt(distance[0] * distance[0] + distance[1] * distance[1] + distance[2] * distance[2]) >=
      minDistance)
      return false;
  }
  {
    double queryPoint[3] = { bounds[1], bounds[2], bounds[5] };
    double* point = dataSet->GetPoint(dataSet->FindPoint(queryPoint));
    double distance[3] = { queryPoint[0] - point[0], queryPoint[1] - point[1],
      queryPoint[2] - point[2] };
    if (sqrt(distance[0] * distance[0] + distance[1] * distance[1] + distance[2] * distance[2]) >=
      minDistance)
      return false;
  }
  {
    double queryPoint[3] = { bounds[1], bounds[3], bounds[5] };
    double* point = dataSet->GetPoint(dataSet->FindPoint(queryPoint));
    double distance[3] = { queryPoint[0] - point[0], queryPoint[1] - point[1],
      queryPoint[2] - point[2] };
    if (sqrt(distance[0] * distance[0] + distance[1] * distance[1] + distance[2] * distance[2]) >=
      minDistance)
      return false;
  }
  return true;
}

vtkDataSet* vtkHyperTreeGridBox::GetSubdividedData(double bounds[6], int level)
{
  if (this->Extractor.size() <= level)
  {
    vtkPVMetaClipDataSet* filterForThisLevel = vtkPVMetaClipDataSet::New();
    filterForThisLevel->SetInputConnection(
      this->Extractor[this->Extractor.size() - 1]->GetOutputPort());
    filterForThisLevel->ExactBoxClipOn();
    filterForThisLevel->SetInsideOut(1);
    this->Extractor.push_back(filterForThisLevel);
  }

  vtkNew<vtkPVBox> box;
  box->SetBounds(bounds);
  static_cast<vtkPVMetaClipDataSet*>(this->Extractor[level])->SetClipFunction(box);

  this->Extractor[level]->Update();
  vtkDataSet* dataSet = vtkDataSet::SafeDownCast(this->Extractor[level]->GetOutputDataObject(0));
  return dataSet;
}

vtkDataArray* vtkHyperTreeGridBox::GetDataObject(vtkDataSet* dataSet, vtkIdType idx)
{
  this->PointDataToCellDataConverter->SetInputData(dataSet);
  this->PointDataToCellDataConverter->Update();
  vtkDataArray* data =
    this->PointDataToCellDataConverter->GetOutput()->GetCellData()->GetArray(idx);

  if (!data)
  {
    vtkNew<vtkDoubleArray> emptyResult;
    emptyResult->SetNumberOfTuples(0);
    return emptyResult;
  }
  return data;
}

// TODO: cache the mean value
double vtkHyperTreeGridBox::Mean(vtkDataArray* data)
{
  size_t numCells = data->GetNumberOfTuples();

  size_t numComponents = data->GetNumberOfComponents();

  double mean = 0.0;
  double* tuple;
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
  {
    switch (numComponents)
    {
      case 1:
        mean += data->GetTuple1(cellId);
        break;
      case 2:
        tuple = data->GetTuple2(cellId);
        mean += sqrt(tuple[0] * tuple[0] + tuple[1] * tuple[1]);
        break;
      case 3:
        tuple = data->GetTuple3(cellId);
        mean += sqrt(tuple[0] * tuple[0] + tuple[1] * tuple[1] + tuple[2] * tuple[2]);
        break;
      default:
        vtkErrorMacro(
          "Mean value for comber of components " << numComponents << " not implemented.");
    }
  }

  return mean / numCells;
}

bool vtkHyperTreeGridBox::Max(vtkDataArray* data, double value)
{
  return this->Mean(data) > value;
}

bool vtkHyperTreeGridBox::Min(vtkDataArray* data, double value)
{
  return this->Mean(data) < value;
}

bool vtkHyperTreeGridBox::Entropy(vtkDataArray* data, double value)
{
  if (data->GetNumberOfTuples() == 1)
  {
    return 1.0;
  }

  static const size_t NUM_BINS = 64;
  int bins[NUM_BINS] = { 0 };

  double range[2] = { 0 };
  vtkIdType numComponents = data->GetNumberOfComponents();
  for (size_t i = 0; i < numComponents; ++i)
  {
    double* rangeInDim = data->GetRange(i);
    range[0] = range[0] < rangeInDim[0] ? range[0] : rangeInDim[0];
    range[1] = range[1] > rangeInDim[1] ? range[1] : rangeInDim[1];
  }
  double binWidth = (range[1] - range[0]) / NUM_BINS;

  if (binWidth == 0.0)
  {
    return 1.0;
  }
  if (data->GetNumberOfTuples() == 2)
  {
    double tuple1 = data->GetTuple1(0);
    double tuple2 = data->GetTuple1(1);
    double entropy = abs(tuple1 - tuple2) <= binWidth ? 1.0 : 2.0;
    return entropy < value;
  }

  double numPoints = data->GetNumberOfTuples();
  double* tuple;
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    switch (numComponents)
    {
      case 1:
      {
        vtkIdType bin = std::floor((data->GetTuple1(i) - range[0]) / binWidth);
        bin = bin == 64 ? 63 : bin;
        assert(bin >= 0 && bin < NUM_BINS);
        ++(bins[bin]);
        break;
      }
      case 2:
      {
        tuple = data->GetTuple2(i);
        vtkIdType bin = std::floor((tuple[0] - range[0]) / binWidth);
        bin = bin == 64 ? 63 : bin;
        assert(bin >= 0 && bin < NUM_BINS);
        ++(bins[bin]);
        bin = std::floor((tuple[1] - range[0]) / binWidth);
        bin = bin == 64 ? 63 : bin;
        assert(bin >= 0 && bin < NUM_BINS);
        ++(bins[bin]);
        break;
      }
      case 3:
      {
        tuple = data->GetTuple3(i);
        vtkIdType bin = std::floor((tuple[0] - range[0]) / binWidth);
        bin = bin == 64 ? 63 : bin;
        assert(bin >= 0 && bin < NUM_BINS);
        ++(bins[bin]);
        bin = std::floor((tuple[1] - range[0]) / binWidth);
        bin = bin == 64 ? 63 : bin;
        assert(bin >= 0 && bin < NUM_BINS);
        ++(bins[bin]);
        bin = std::floor((tuple[2] - range[0]) / binWidth);
        bin = bin == 64 ? 63 : bin;
        assert(bin >= 0 && bin < NUM_BINS);
        ++(bins[bin]);
        break;
      }
    }
  }

  double entropy = 0.0;
  for (size_t i = 0; i < NUM_BINS; ++i)
  {
    double e = bins[i] > 0 ? -bins[i] / numPoints * std::log2(bins[i] / numPoints) : 0.0;
    assert("entropy is negative" && e >= 0.0);
    entropy += e;
  }

  return entropy < value;
}

bool vtkHyperTreeGridBox::Range(vtkDataArray* data, double value)
{
  vtkIdType numComponents = data->GetNumberOfComponents();
  vtkIdType numPoints = data->GetNumberOfTuples();
  double max = 0;
  double min = VTK_FLOAT_MAX;
  double* tuple;
  double magnitude;
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    switch (numComponents)
    {
      case 1:
        magnitude = data->GetTuple1(i);
        break;
      case 2:
        tuple = data->GetTuple2(i);
        magnitude = sqrt(tuple[0] * tuple[0] + tuple[1] * tuple[1]);
        break;
      case 3:
        tuple = data->GetTuple3(i);
        magnitude = sqrt(tuple[0] * tuple[0] + tuple[1] * tuple[1] + tuple[2] * tuple[2]);
    }
    max = max < magnitude ? magnitude : max;
    min = min > magnitude ? magnitude : min;
  }
  return abs(max - min) < value;
}

int vtkHyperTreeGridBox::MaskImplementation(vtkDataSet* dataSet)
{
  return !dataSet->GetNumberOfPoints() || !dataSet->GetNumberOfCells();
}

bool vtkHyperTreeGridBox::MetricImplementation(vtkDataArray* data)
{
  bool ret = false;
  if (this->GetUseMin())
  {
    ret |= this->Min(data, this->GetMinValue());
    if (ret)
      return true;
  }
  if (this->GetUseMax())
  {
    ret |= this->Max(data, this->GetMaxValue());
    if (ret)
      return true;
  }
  if (this->GetUseEntropy())
  {
    ret |= this->Entropy(data, this->GetEntropyValue());
    if (ret)
      return true;
  }
  if (this->GetUseRange())
  {
    ret |= this->Range(data, this->GetRangeValue());
    if (ret)
      return true;
  }
  return ret;
}
