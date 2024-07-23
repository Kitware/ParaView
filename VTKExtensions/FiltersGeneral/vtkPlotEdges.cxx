// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPlotEdges.h"

#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCleanPolyData.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkConvertToMultiBlockDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkReductionFilter.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkType.h"

//-----------------------------------------------------------------------------
class vtkPlotEdges::Segment : public vtkObject
{
  vtkTypeMacro(Segment, vtkObject);
  static Segment* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetObjectMacro(PolyData, vtkPolyData);
  vtkGetObjectMacro(PolyData, vtkPolyData);

  void AddPoint(vtkIdType cellId, vtkIdType pointId);
  void InsertSegment(vtkIdType pos, Segment* segment);

  vtkGetObjectMacro(PointIdList, vtkIdList);
  unsigned int GetCountPointIds() { return this->PointIdList->GetNumberOfIds(); }

  double GetLength() const;
  vtkGetObjectMacro(ArcLengths, vtkDoubleArray);

  vtkGetMacro(StartId, vtkIdType);
  vtkGetMacro(EndId, vtkIdType);

  double* GetStartPoint(double* point) const;
  double* GetEndPoint(double* point) const;

  const double* GetStartDirection() const;
  const double* GetEndDirection() const;
  double* GetDirection(vtkIdType pointId, double* direction) const;

protected:
  Segment();
  ~Segment() override;

  void ComputeDirection(vtkIdType pointIndex, bool increment, double* direction) const;

  vtkPolyData* PolyData;

  vtkIdType StartId;
  vtkIdType EndId;

  vtkIdList* PointIdList;
  vtkDoubleArray* ArcLengths;

  mutable double StartDirection[3];
  mutable double EndDirection[3];

private:
  Segment(const vtkObject&) = delete;
  void operator=(const Segment&) = delete;
};

class vtkPlotEdges::Node : public vtkObject
{
  vtkTypeMacro(Node, vtkObject);
  static Node* New();

  vtkSetObjectMacro(PolyData, vtkPolyData);
  vtkGetObjectMacro(PolyData, vtkPolyData);

  vtkSetMacro(PointId, vtkIdType);
  vtkGetMacro(PointId, vtkIdType);

  void AddSegment(Segment* segment);
  vtkGetObjectMacro(Segments, vtkCollection);

  double ComputeConnectionScore(Segment* segment1, Segment* segment2);

protected:
  Node();
  ~Node() override;

  vtkPolyData* PolyData;
  vtkIdType PointId;
  vtkCollection* Segments;

private:
  Node(const vtkObject&) = delete;
  void operator=(const Node&) = delete;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotEdges::Segment);

//-----------------------------------------------------------------------------
vtkPlotEdges::Segment::Segment()
{
  this->PolyData = nullptr;

  this->StartId = -1;
  this->EndId = -1;

  this->PointIdList = vtkIdList::New();

  this->ArcLengths = vtkDoubleArray::New();
  this->ArcLengths->SetName("arc_length");
  this->ArcLengths->SetNumberOfComponents(1);

  this->StartDirection[0] = 0.;
  this->StartDirection[1] = 0.;
  this->StartDirection[2] = 0.;
  this->EndDirection[0] = 0.;
  this->EndDirection[1] = 0.;
  this->EndDirection[2] = 0.;
}

//-----------------------------------------------------------------------------
vtkPlotEdges::Segment::~Segment()
{
  if (this->PolyData)
  {
    this->PolyData->Delete();
  }
  this->PointIdList->Delete();
  this->ArcLengths->Delete();
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::Segment::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "vtkPolyData: " << this->PolyData << endl;
  os << indent << "StartId: " << this->StartId << endl;
  os << indent << "EndId: " << this->EndId << endl;
  os << indent << "Num Points" << this->PointIdList->GetNumberOfIds() << endl;
  os << indent << "Length" << this->GetLength() << endl;
  const double* direction = this->GetStartDirection();
  os << indent << "StartDirection: " << direction[0] << "," << direction[1] << "," << direction[2]
     << endl;
  direction = this->GetEndDirection();
  os << indent << "EndDirection: " << direction[0] << "," << direction[1] << "," << direction[2]
     << endl;
}

//-----------------------------------------------------------------------------
double vtkPlotEdges::Segment::GetLength() const
{
  // return m_Length;
  if (this->ArcLengths->GetMaxId() != -1)
  {
    return this->ArcLengths->GetValue(this->ArcLengths->GetMaxId());
  }
  return 0.;
}

//-----------------------------------------------------------------------------
double* vtkPlotEdges::Segment::GetStartPoint(double* point) const
{
  const_cast<vtkPolyData*>(this->PolyData)->GetPoint(this->StartId, point);
  return point;
}

//-----------------------------------------------------------------------------
double* vtkPlotEdges::Segment::GetEndPoint(double* point) const
{
  const_cast<vtkPolyData*>(this->PolyData)->GetPoint(this->EndId, point);
  return point;
}

//-----------------------------------------------------------------------------
const double* vtkPlotEdges::Segment::GetStartDirection() const
{
  if (this->StartDirection[0] == 0. && this->StartDirection[1] == 0. &&
    this->StartDirection[2] == 0.)
  {
    this->ComputeDirection(0, true, this->StartDirection);
  }
  this->ComputeDirection(0, true, this->StartDirection);
  return this->StartDirection;
}

//-----------------------------------------------------------------------------
const double* vtkPlotEdges::Segment::GetEndDirection() const
{
  if (this->EndDirection[0] == 0. && this->EndDirection[1] == 0. && this->EndDirection[2] == 0.)
  {
    this->ComputeDirection(this->PointIdList->GetNumberOfIds() - 1, false, this->EndDirection);
  }
  this->ComputeDirection(this->PointIdList->GetNumberOfIds() - 1, false, this->EndDirection);
  return this->EndDirection;
}

//-----------------------------------------------------------------------------
double* vtkPlotEdges::Segment::GetDirection(vtkIdType pointId, double* direction) const
{
  if (pointId == this->StartId)
  {
    memcpy(direction, this->GetStartDirection(), 3 * sizeof(double));
  }
  else if (pointId == this->EndId)
  {
    memcpy(direction, this->GetEndDirection(), 3 * sizeof(double));
  }
  else
  {
    // warning, if the segment makes a loop, points may be listed
    // more than once. this->PointIdList->GetId returns the first corresponding point
    this->ComputeDirection(this->PointIdList->IsId(pointId), true, direction);
  }
  return direction;
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::Segment::ComputeDirection(
  vtkIdType pointIndex, bool increment, double* direction) const
{
  direction[0] = 0.;
  direction[1] = 0.;
  direction[2] = 0.;

  // Point1
  // vtkIdType pointIndex = this->PointIdList->IsId(pointId);
  vtkIdType pointId = this->PointIdList->GetId(pointIndex);
  if (pointIndex == -1 || pointId == -1)
  {
    return;
  }

  double point1[3];

  this->PolyData->GetPoint(pointId, point1);

  // Point2
  double point2[3];
  pointIndex += increment ? 1 : -1;
  pointId = this->PointIdList->GetId(pointIndex);
  if (pointIndex == -1 || pointIndex >= this->PointIdList->GetNumberOfIds())
  {
    // not enough points to compute the direction
    return;
  }
  this->PolyData->GetPoint(pointId, point2);

  // vector21
  double vector21[3];
  vector21[0] = point1[0] - point2[0];
  vector21[1] = point1[1] - point2[1];
  vector21[2] = point1[2] - point2[2];

  double length = vtkMath::Norm(vector21);
  double averageLength = this->GetLength() / this->PointIdList->GetNumberOfIds();
  while (averageLength > length)
  {
    direction[0] += vector21[0];
    direction[1] += vector21[1];
    direction[2] += vector21[2];
    averageLength -= length;

    // Go to next vector
    memcpy(point1, point2, 3 * sizeof(double));
    pointIndex += increment ? 1 : -1;
    pointId = this->PointIdList->GetId(pointIndex);
    if (pointIndex == -1 || pointIndex > this->PointIdList->GetNumberOfIds())
    {
      return;
    }
    this->PolyData->GetPoint(pointId, point2);
    vector21[0] = point1[0] - point2[0];
    vector21[1] = point1[1] - point2[1];
    vector21[2] = point1[2] - point2[2];
    length = vtkMath::Norm(vector21);
  }
  if (length > 0.0000001)
  {
    direction[0] += vector21[0] * (averageLength / length);
    direction[1] += vector21[1] * (averageLength / length);
    direction[2] += vector21[2] * (averageLength / length);
  }
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::Segment::AddPoint(vtkIdType vtkNotUsed(cellId), vtkIdType pointId)
{
  if (this->StartId == -1)
  {
    this->StartId = pointId;
  }

  double newPoint[3], currentPoint[3];

  if (this->EndId != -1)
  {
    this->PolyData->GetPoint(pointId, newPoint);
    this->PolyData->GetPoint(this->EndId, currentPoint);
  }
  else
  {
    this->PolyData->GetPoint(pointId, newPoint);
    memcpy(currentPoint, newPoint, 3 * sizeof(double));
  }

  this->EndId = pointId;
  this->PointIdList->InsertNextId(pointId);

  double length = sqrt(vtkMath::Distance2BetweenPoints(currentPoint, newPoint));

  if (this->ArcLengths->GetMaxId() != -1)
  {
    length += this->ArcLengths->GetValue(this->ArcLengths->GetMaxId());
  }
  this->ArcLengths->InsertNextValue(length);

  // reset directions
  this->StartDirection[0] = 0.;
  this->StartDirection[1] = 0.;
  this->StartDirection[2] = 0.;
  this->EndDirection[0] = 0.;
  this->EndDirection[1] = 0.;
  this->EndDirection[2] = 0.;
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::Segment::InsertSegment(vtkIdType pos, Segment* segment)
{
  if (segment->PolyData != this->PolyData)
  {
    return;
  }
  // GetLength() can change during InsertSegment, we save the result here.
  double length = this->GetLength();

  // What is the point in common between the 2 segments.
  // that point has to be updated with the other segment extremity.
  if (this->StartId == pos)
  {
    vtkIdList* unionList = vtkIdList::New();
    vtkDoubleArray* arcLengths = vtkDoubleArray::New();
    arcLengths->SetName(this->ArcLengths->GetName());
    arcLengths->SetNumberOfComponents(1);

    // the segment is concatenated at the beginning.
    if (segment->StartId == pos)
    {
      // the list must be invertly copied
      this->StartId = segment->EndId;
      for (vtkIdType i = segment->PointIdList->GetNumberOfIds() - 1; i != -1; --i)
      {
        unionList->InsertNextId(segment->PointIdList->GetId(i));
        arcLengths->InsertNextValue(segment->GetLength() - segment->ArcLengths->GetValue(i));
      }
    }
    else
    {
      this->StartId = segment->StartId;
      for (vtkIdType i = 0; i < segment->PointIdList->GetNumberOfIds(); ++i)
      {
        unionList->InsertNextId(segment->PointIdList->GetId(i));
        arcLengths->InsertNextValue(segment->ArcLengths->GetValue(i));
      }
    }

    // copy the point list
    for (vtkIdType i = 1; i < this->PointIdList->GetNumberOfIds(); ++i)
    {
      unionList->InsertNextId(this->PointIdList->GetId(i));
      arcLengths->InsertNextValue(this->ArcLengths->GetValue(i) + segment->GetLength());
    }

    this->PointIdList->DeepCopy(unionList);
    this->ArcLengths->DeepCopy(arcLengths);

    unionList->Delete();
    arcLengths->Delete();
  }
  else
  {
    // the segment is concatenated at the end.
    if (segment->StartId == pos)
    {
      this->EndId = segment->EndId;
      for (vtkIdType i = 1; i < segment->PointIdList->GetNumberOfIds(); ++i)
      {
        this->PointIdList->InsertNextId(segment->PointIdList->GetId(i));
        this->ArcLengths->InsertNextValue(segment->ArcLengths->GetValue(i) + length);
      }
    }
    else
    {
      this->EndId = segment->StartId;
      for (vtkIdType i = segment->PointIdList->GetNumberOfIds() - 2; i != -1; --i)
      {
        this->PointIdList->InsertNextId(segment->PointIdList->GetId(i));
        this->ArcLengths->InsertNextValue(
          segment->GetLength() - segment->ArcLengths->GetValue(i) + length);
      }
    }
  }

  this->StartDirection[0] = 0.;
  this->StartDirection[1] = 0.;
  this->StartDirection[2] = 0.;
  this->EndDirection[0] = 0.;
  this->EndDirection[1] = 0.;
  this->EndDirection[2] = 0.;
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotEdges::Node);

//-----------------------------------------------------------------------------
vtkPlotEdges::Node::Node()
{
  this->PolyData = nullptr;
  this->PointId = -1;
  this->Segments = vtkCollection::New();
}

//-----------------------------------------------------------------------------
vtkPlotEdges::Node::~Node()
{
  if (this->PolyData)
  {
    this->PolyData->Delete();
  }
  this->Segments->Delete();
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::Node::AddSegment(Segment* segment)
{
  this->Segments->AddItem(segment);
}

//-----------------------------------------------------------------------------
double vtkPlotEdges::Node::ComputeConnectionScore(Segment* segment1, Segment* segment2)
{
  if (segment1 == segment2)
  {
    return -1.;
  }

  //        (a.b + 1)     1 -  | ||a||-||b|| |
  // Score =  -------  *(     ----------------- )
  //             2             max(||a||, ||b||)
  // The perfect score(1)is an opposite direction and an
  // identique point frequency
  // add a penalty if the 2 extremities are the same
  double segment1Direction[3], segment2Direction[3];
  segment1->GetDirection(this->PointId, segment1Direction);
  segment2->GetDirection(this->PointId, segment2Direction);

  double segment1DirectionNorm = vtkMath::Normalize(segment1Direction);
  double segment2DirectionNorm = vtkMath::Normalize(segment2Direction);

  double angleScore = (1. - vtkMath::Dot(segment1Direction, segment2Direction)) / 2.;
  double pointFrequencyScore = 1. -
    std::abs(segment1DirectionNorm - segment2DirectionNorm) /
      std::max(segment1DirectionNorm, segment2DirectionNorm);
  double penaltyScore = 1.;
  // prevent small loops

  double start1[3], end1[3];
  double start2[3], end2[3];
  if (segment1->GetCountPointIds() <= 3 &&
    ((segment1->GetStartId() == segment2->GetStartId() &&
       segment1->GetEndId() == segment2->GetEndId()) ||
      (segment1->GetStartId() == segment2->GetEndId() &&
        segment1->GetEndId() == segment2->GetStartId())))
  {
    penaltyScore = 0.4;
  }
  else
  {
    segment1->GetStartPoint(start1);
    segment1->GetEndPoint(end1);
    segment2->GetStartPoint(start2);
    segment2->GetEndPoint(end2);
    if (segment1->GetCountPointIds() <= 3 &&
      ((vtkMath::Distance2BetweenPoints(start1, start2) < 0.00001 &&
         vtkMath::Distance2BetweenPoints(end1, end2) < 0.00001) ||
        (vtkMath::Distance2BetweenPoints(start1, end2) < 0.00001 &&
          vtkMath::Distance2BetweenPoints(end1, start2) < 0.00001)))
    {
      penaltyScore = 0.45;
    }
  }

  return angleScore * pointFrequencyScore * penaltyScore;
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotEdges);

//-----------------------------------------------------------------------------
vtkPlotEdges::vtkPlotEdges()
{
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkPlotEdges::~vtkPlotEdges() = default;

//-----------------------------------------------------------------------------
int vtkPlotEdges::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkPlotEdges::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject* inputDO = vtkDataObject::GetData(inInfo);
  vtkMultiBlockDataSet* outputMB = vtkMultiBlockDataSet::GetData(outInfo);

  if (auto inputPolyData = vtkPolyData::SafeDownCast(inputDO))
  {
    this->Process(inputPolyData, outputMB);
    return 1;
  }
  // This is for vtkMultiPieceDataSet and vtkPartitionedDataSet
  else if (auto inputPDS = vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    this->ProcessPartitionedDataSet(inputPDS, outputMB);
    return 1;
  }
  else if (auto inputMB = vtkMultiBlockDataSet::SafeDownCast(inputDO))
  {
    this->ProcessMultiBlockDataSet(inputMB, outputMB);
    return 1;
  }
  else if (auto inputPDSC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO))
  {
    vtkNew<vtkConvertToMultiBlockDataSet> convert;
    convert->SetInputData(inputPDSC);
    convert->Update();
    this->ProcessMultiBlockDataSet(convert->GetOutput(), outputMB);
    return 1;
  }
  else
  {
    vtkErrorMacro("Input data type of '" << inputDO->GetClassName() << "' is not supported yet.");
    return 0;
  }
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::ProcessMultiBlockDataSet(
  vtkMultiBlockDataSet* input, vtkMultiBlockDataSet* output)
{
  output->SetNumberOfBlocks(input->GetNumberOfBlocks());
  for (unsigned int i = 0; i < input->GetNumberOfBlocks(); ++i)
  {
    auto inputBlockDO = input->GetBlock(i);
    if (inputBlockDO == nullptr)
    {
      continue;
    }
    std::string name =
      input->HasMetaData(i) && input->GetMetaData(i)->Get(vtkCompositeDataSet::NAME())
      ? input->GetMetaData(i)->Get(vtkCompositeDataSet::NAME())
      : "Block_" + std::to_string(i);
    if (auto inputBlockMBS = vtkMultiBlockDataSet::SafeDownCast(inputBlockDO))
    {
      vtkNew<vtkMultiBlockDataSet> outputBlockMB;
      output->SetBlock(i, outputBlockMB);
      output->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), name);
      this->ProcessMultiBlockDataSet(inputBlockMBS, outputBlockMB);
    }
    else if (auto inputBlockPDS = vtkPartitionedDataSet::SafeDownCast(inputBlockDO))
    {
      vtkNew<vtkMultiBlockDataSet> outputBlockMB;
      output->SetBlock(i, outputBlockMB);
      output->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), name);
      this->ProcessPartitionedDataSet(inputBlockPDS, outputBlockMB);
    }
    else if (auto inputBlockPD = vtkPolyData::SafeDownCast(inputBlockDO))
    {
      vtkNew<vtkMultiBlockDataSet> outputBlockMB;
      output->SetBlock(i, outputBlockMB);
      output->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), name);
      this->Process(inputBlockPD, outputBlockMB);
    }
    else
    {
      vtkErrorMacro(
        "Input data type of '" << inputBlockDO->GetClassName() << "' is not supported.");
    }
  }
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::ProcessPartitionedDataSet(
  vtkPartitionedDataSet* input, vtkMultiBlockDataSet* outputMB)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  outputMB->SetNumberOfBlocks(input->GetNumberOfPartitions());
  for (unsigned int i = 0; i < input->GetNumberOfPartitions(); ++i)
  {
    if (auto partitionPD = vtkPolyData::SafeDownCast(input->GetPartition(i)))
    {
      vtkNew<vtkMultiBlockDataSet> tempMB;
      std::string name =
        input->HasMetaData(i) && input->GetMetaData(i)->Has(vtkCompositeDataSet::NAME())
        ? input->GetMetaData(i)->Get(vtkCompositeDataSet::NAME())
        : "dataset_" + std::to_string(i);
      this->Process(partitionPD, tempMB);
      if (controller->GetLocalProcessId() == 0)
      {
        outputMB->SetBlock(i, tempMB);
      }
      outputMB->GetMetaData(i)->Set(vtkCompositeDataSet::NAME(), name);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::Process(vtkPolyData* input, vtkMultiBlockDataSet* outputMB)
{
  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  ReducePolyData(input, inputPolyData);

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  if (controller->GetLocalProcessId() > 0)
  {
    int numberOfBlocks = 0;
    controller->Broadcast(&numberOfBlocks, 1, 0);
    // to ensure structure matches on all processes.
    outputMB->SetNumberOfBlocks(numberOfBlocks);
  }
  else
  {
    vtkNew<vtkCollection> segments;
    vtkNew<vtkCollection> nodes;
    this->ExtractSegments(inputPolyData, segments, nodes);
    this->ConnectSegmentsWithNodes(segments, nodes);
    this->SaveToMultiBlockDataSet(segments, outputMB);

    int numberofBlocks = outputMB->GetNumberOfBlocks();
    controller->Broadcast(&numberofBlocks, 1, 0);
  }
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::ReducePolyData(vtkPolyData* polyData, vtkPolyData* output)
{
  // Gather all the data from the different processors to run the
  // filter on the 1st root processor.
  vtkSmartPointer<vtkReductionFilter> md = vtkSmartPointer<vtkReductionFilter>::New();

  md->SetController(vtkMultiProcessController::GetGlobalController());

  vtkSmartPointer<vtkAppendPolyData> as = vtkSmartPointer<vtkAppendPolyData>::New();
  md->SetPostGatherHelper(as);

  vtkSmartPointer<vtkPolyData> inputPD = vtkSmartPointer<vtkPolyData>::New();
  inputPD->ShallowCopy(vtkPolyData::SafeDownCast(polyData));
  md->SetInputData(inputPD);

  md->Update();

  output->ShallowCopy(vtkPolyData::SafeDownCast(md->GetOutputDataObject(0)));

  // vtkPlotEdges cannot handle ghost-cells at all. Remove all ghost cells
  // BUG #12422.
  output->RemoveGhostCells();
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::ExtractSegments(
  vtkPolyData* input, vtkCollection* segments, vtkCollection* nodes)
{
  vtkSmartPointer<vtkCleanPolyData> cleanPolyData = vtkSmartPointer<vtkCleanPolyData>::New();
  cleanPolyData->SetInputData(input);
  // set the tolerance at 0 to use vtkMergePoints (faster)
  cleanPolyData->SetTolerance(0.0);
  cleanPolyData->Update();

  vtkPolyData* polyData = cleanPolyData->GetOutput();
  polyData->BuildLinks();

  int abort = 0;
  vtkIdType numCells = polyData->GetNumberOfCells();
  vtkIdType progressInterval = numCells / 20 + 1;

  char* visitedCells = new char[numCells];
  memset(visitedCells, 0, numCells * sizeof(char));

  for (vtkIdType cellId = 0; cellId < numCells && !abort; cellId++)
  {
    if (!(cellId % progressInterval))
    {
      // Give/Get some feedbacks for/from the user
      this->UpdateProgress(static_cast<float>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    if (visitedCells[cellId])
    {
      // the cell has already been visited, go to the next
      continue;
    }

    if (polyData->GetCellType(cellId) != VTK_LINE && polyData->GetCellType(cellId) != VTK_POLY_LINE)
    {
      // No other types than VTK_LINE and VTK_POLY_LINE is handled
      continue;
    }

    vtkIdType numCellPts;
    const vtkIdType* cellPts;

    // We take the first point from a cell and start to track
    // all the branches from it.
    polyData->GetCellPoints(cellId, numCellPts, cellPts);
    if (numCellPts != 2)
    {
      continue;
    }

    vtkIdType numPtCells;
    vtkIdType* cellIds;

    // A point can be used by many cells, we get them all
    // and track the branches from each cells
    polyData->GetPointCells(cellPts[0], numPtCells, cellIds);

    // As the point may be in many cells, we need to create a node.
    // The nodes will be merged later in ConnectSegmentsWithNodes
    Node* node = nullptr;
    if (numPtCells > 1)
    {
      node = Node::New();
      node->SetPolyData(polyData);
      node->SetPointId(cellPts[0]);
      nodes->AddItem(node);
      node->Delete();
    }

    // Track the branchs from the point. The cell defines a direction
    for (vtkIdType i = 0; i < numPtCells; ++i)
    {
      this->ExtractSegmentsFromExtremity(
        polyData, segments, nodes, visitedCells, cellIds[i], cellPts[0], node);
    }
  }
  // we don't need the visited array array
  delete[] visitedCells;
  visitedCells = nullptr;
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::ExtractSegmentsFromExtremity(vtkPolyData* polyData, vtkCollection* segments,
  vtkCollection* nodes, char* visitedCells, vtkIdType cellId, vtkIdType pointId, Node* node)
{
  if (visitedCells[cellId])
  {
    return;
  }
  if (polyData->GetCellType(cellId) != VTK_LINE && polyData->GetCellType(cellId) != VTK_POLY_LINE)
  {
    return;
  }

  // Get all the points from the cell
  vtkIdType numCellPts;
  const vtkIdType* cellPts;

  polyData->GetCellPoints(cellId, numCellPts, cellPts);

  if (numCellPts != 2)
  {
    return;
  }

  // Get the next point
  vtkIdType pointId2 = (cellPts[0] == pointId) ? cellPts[1] : cellPts[0];

  double p[3];
  polyData->GetPoint(pointId2, p);

  // Create a new segment from the point.
  Segment* segment = Segment::New();
  segment->SetPolyData(polyData);
  segment->AddPoint(cellId, pointId);
  segment->AddPoint(cellId, pointId2);

  // if the pointId is at a node, add the segment to it
  // maybe node shall not be given as a parameter but
  // get using vtkPlotEdges::GetNodeAtPoint()
  if (node)
  {
    node->AddSegment(segment);
  }

  segments->AddItem(segment);
  segment->Delete();

  visitedCells[cellId] = 1;

  // Get all the cells associated with the point2
  vtkIdType numPtCells;
  vtkIdType* cellIds;

  polyData->GetPointCells(pointId2, numPtCells, cellIds);
  while (numPtCells != 1) // 1 is the end of a segment
  {
    if (numPtCells > 2)
    {
      // we are at a tree branch node
      Node* node2 = vtkPlotEdges::GetNodeAtPoint(nodes, pointId2);
      if (!node2)
      {
        node2 = Node::New();
        node2->SetPolyData(polyData);
        node2->SetPointId(pointId2);
        nodes->AddItem(node2);
        node2->Delete();
      }
      node2->AddSegment(segment);

      for (vtkIdType i = 0; i < numPtCells; ++i)
      {
        if (!visitedCells[cellIds[i]] &&
          (polyData->GetCellType(cellIds[i]) == VTK_LINE ||
            polyData->GetCellType(cellIds[i]) == VTK_POLY_LINE))
        {
          vtkPlotEdges::ExtractSegmentsFromExtremity(
            polyData, segments, nodes, visitedCells, cellIds[i], pointId2, node2);
        }
      }
      // we were at the extremity of a branch, like if numptCells == 1
      break;
    }
    else
    {
      // get the next cell
      vtkIdType cellId2 = (cellIds[0] == cellId) ? cellIds[1] : cellIds[0];
      if (visitedCells[cellId2])
      {
        break;
      }
      if (polyData->GetCellType(cellId2) != VTK_LINE &&
        polyData->GetCellType(cellId2) != VTK_POLY_LINE)
      {
        // need to do something smarter
        break;
      }
      vtkIdType numCell2Pts;
      const vtkIdType* cell2Pts;
      polyData->GetCellPoints(cellId2, numCell2Pts, cell2Pts);

      if (numCell2Pts != 2)
      {
        // need to do something smarter
        break;
      }
      // get the next point
      vtkIdType pointId3 = (cell2Pts[0] == pointId2) ? cell2Pts[1] : cell2Pts[0];
      segment->AddPoint(cellId2, pointId3);

      // go one step forward
      visitedCells[cellId2] = 1;
      cellId = cellId2;
      pointId2 = pointId3;

      polyData->GetPointCells(pointId2, numPtCells, cellIds);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::ConnectSegmentsWithNodes(vtkCollection* segments, vtkCollection* nodes)
{
  Node* node = nullptr;
  vtkCollectionIterator* nodeIt = nodes->NewIterator();
  // do a first pass with straightforward nodes(2 branches)
  nodeIt->GoToFirstItem();
  while (!nodeIt->IsDoneWithTraversal())
  {
    node = Node::SafeDownCast(nodeIt->GetCurrentObject());
    if (node->GetSegments()->GetNumberOfItems() == 2)
    {
      Segment* segmentA = Segment::SafeDownCast(node->GetSegments()->GetItemAsObject(0));
      Segment* segmentB = Segment::SafeDownCast(node->GetSegments()->GetItemAsObject(1));
      vtkPlotEdges::MergeSegments(segments, nodes, node, segmentA, segmentB);
      nodeIt->GoToNextItem();
      nodes->RemoveItem(node);
    }
    else
    {
      nodeIt->GoToNextItem();
    }
  }

  // do a second pass with the other nodes
  nodeIt->GoToFirstItem();
  while (!nodeIt->IsDoneWithTraversal())
  {
    node = Node::SafeDownCast(nodeIt->GetCurrentObject());
    double point[3];
    node->GetPolyData()->GetPoint(node->GetPointId(), point);

    while (node->GetSegments()->GetNumberOfItems() > 1)
    {
      vtkCollectionIterator* it = node->GetSegments()->NewIterator();
      vtkCollectionIterator* it2 = node->GetSegments()->NewIterator();

      Segment* segmentI = nullptr;
      Segment* segmentJ = nullptr;
      Segment* segmentA = nullptr;
      Segment* segmentB = nullptr;

      double old_score = -2.;
      double score = 0;

      for (it->GoToFirstItem(); (segmentI = Segment::SafeDownCast(it->GetCurrentObject()));
           it->GoToNextItem())
      {
        for (it2->GoToFirstItem(); (segmentJ = Segment::SafeDownCast(it2->GetCurrentObject()));
             it2->GoToNextItem())
        {
          score = node->ComputeConnectionScore(segmentI, segmentJ);

          if (score > old_score)
          {
            segmentA = segmentI;
            segmentB = segmentJ;
            old_score = score;
          }
        }
      }
      vtkPlotEdges::MergeSegments(segments, nodes, node, segmentA, segmentB);

      it->Delete();
      it2->Delete();
    }

    nodes->RemoveItem(node);
    nodeIt->GoToFirstItem();
  }
  nodeIt->Delete();
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::MergeSegments(
  vtkCollection* segments, vtkCollection* nodes, Node* node, Segment* segmentA, Segment* segmentB)
{
  if (segmentA == segmentB)
  {
    node->GetSegments()->RemoveItem(segmentA);
    node->GetSegments()->RemoveItem(segmentB);
    return;
  }

  segmentA->InsertSegment(node->GetPointId(), segmentB);
  node->GetSegments()->RemoveItem(segmentA);
  node->GetSegments()->RemoveItem(segmentB);

  // replace segmentB by segmentA in other nodes
  vtkCollectionIterator* nodeIt = nodes->NewIterator();
  for (nodeIt->GoToFirstItem(); !nodeIt->IsDoneWithTraversal(); nodeIt->GoToNextItem())
  {
    Node* node2 = Node::SafeDownCast(nodeIt->GetCurrentObject());
    int segmentBPos = node2->GetSegments()->IsItemPresent(segmentB);
    if (segmentBPos)
    {
      node2->GetSegments()->ReplaceItem(segmentBPos - 1, segmentA);
    }
  }
  nodeIt->Delete();
  segments->RemoveItem(segmentB);
}

//-----------------------------------------------------------------------------
vtkPlotEdges::Node* vtkPlotEdges::GetNodeAtPoint(vtkCollection* nodes, vtkIdType pointId)
{
  vtkCollectionIterator* it = nodes->NewIterator();
  Node* res = nullptr;
  for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    Node* node = Node::SafeDownCast(it->GetCurrentObject());
    if (node->GetPointId() == pointId)
    {
      res = node;
      break;
    }
  }
  it->Delete();
  return res;
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::SaveToMultiBlockDataSet(vtkCollection* segments, vtkMultiBlockDataSet* outputMB)
{
  // copy into dataset
  segments->InitTraversal();
  Segment* segment = nullptr;
  outputMB->SetNumberOfBlocks(segments->GetNumberOfItems());
  int cc = 0;
  for (segment = Segment::SafeDownCast(segments->GetNextItemAsObject()); segment;
       segment = Segment::SafeDownCast(segments->GetNextItemAsObject()))
  {
    vtkPolyData* polyData = segment->GetPolyData();

    vtkNew<vtkPolyData> pd;
    std::string partitionName = "segment_" + std::to_string(cc);
    outputMB->GetMetaData(cc)->Set(vtkCompositeDataSet::NAME(), partitionName);
    outputMB->SetBlock(cc++, pd);

    vtkNew<vtkCellArray> ca;

    vtkNew<vtkPoints> pts;
    pts->SetDataType(polyData->GetPoints()->GetDataType());

    vtkNew<vtkIdList> cells;

    vtkPointData* srcPointData = polyData->GetPointData();
    vtkAbstractArray *data, *newData;
    int numArray = srcPointData->GetNumberOfArrays();
    for (int i = 0; i < numArray; i++)
    {
      data = srcPointData->GetAbstractArray(i);
      newData = data->NewInstance(); // instantiate same type of object
      newData->SetNumberOfComponents(data->GetNumberOfComponents());
      newData->SetName(data->GetName());
      if (data->HasInformation())
      {
        newData->CopyInformation(data->GetInformation(), /*deep=*/1);
      }
      pd->GetPointData()->AddArray(newData);
      newData->Delete();
    }

    vtkIdType pointId;
    vtkIdType numCells = segment->GetPointIdList()->GetNumberOfIds();
    for (vtkIdType i = 0; i < numCells; ++i)
    {
      cells->InsertNextId(i);
      pointId = segment->GetPointIdList()->GetId(i);
      pts->InsertPoint(i, polyData->GetPoint(pointId));
      for (int j = 0; j < numArray; j++)
      {
        pd->GetPointData()->GetArray(j)->InsertNextTuple(pointId, srcPointData->GetArray(j));
      }
    }

    pd->SetLines(ca);
    pd->SetPoints(pts);
    pd->InsertNextCell(VTK_POLY_LINE, cells);

    vtkDataArray* arcLength = segment->GetArcLengths();
    if (pd->GetPointData()->HasArray("arc_length"))
    {
      arcLength->SetName("PlotEdges arc_length");
    }
    pd->GetPointData()->AddArray(arcLength);
  }
}

//-----------------------------------------------------------------------------
void vtkPlotEdges::PrintSegments(vtkCollection* segments)
{
  vtkSmartPointer<vtkCollectionIterator> it = segments->NewIterator();
  for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    Segment* segment = Segment::SafeDownCast(it->GetCurrentObject());
    segment->Print(std::cout);
  }
}
