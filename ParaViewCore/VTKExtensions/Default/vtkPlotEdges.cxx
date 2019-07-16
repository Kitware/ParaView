/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkPlotEdges.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCleanPolyData.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkType.h"

#include "vtkAppendCompositeDataLeaves.h"
#include "vtkAppendPolyData.h"
#include "vtkMultiProcessController.h"
#include "vtkReductionFilter.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#define MY_MAX(x, y) ((x) < (y) ? (y) : (x))

class Segment : public vtkObject
{
  vtkTypeMacro(Segment, vtkObject);
  static Segment* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetObjectMacro(PolyData, vtkPolyData);
  vtkGetObjectMacro(PolyData, vtkPolyData);

  void AddPoint(vtkIdType cellId, vtkIdType pointId);
  void InsertSegment(vtkIdType pos, Segment* segment);

  // vtkIdList* GetPointIdList();
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

class Node : public vtkObject
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

vtkStandardNewMacro(Segment);

Segment::Segment()
{
  this->PolyData = NULL;

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

Segment::~Segment()
{
  if (this->PolyData)
  {
    this->PolyData->Delete();
  }
  this->PointIdList->Delete();
  this->ArcLengths->Delete();
}

void Segment::PrintSelf(ostream& os, vtkIndent indent)
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

double Segment::GetLength() const
{
  // return m_Length;
  if (this->ArcLengths->GetMaxId() != -1)
  {
    return this->ArcLengths->GetValue(this->ArcLengths->GetMaxId());
  }
  return 0.;
}

double* Segment::GetStartPoint(double* point) const
{
  const_cast<vtkPolyData*>(this->PolyData)->GetPoint(this->StartId, point);
  return point;
}

double* Segment::GetEndPoint(double* point) const
{
  const_cast<vtkPolyData*>(this->PolyData)->GetPoint(this->EndId, point);
  return point;
}

const double* Segment::GetStartDirection() const
{
  if (this->StartDirection[0] == 0. && this->StartDirection[1] == 0. &&
    this->StartDirection[2] == 0.)
  {
    this->ComputeDirection(0, true, this->StartDirection);
  }
  this->ComputeDirection(0, true, this->StartDirection);
  return this->StartDirection;
}

const double* Segment::GetEndDirection() const
{
  if (this->EndDirection[0] == 0. && this->EndDirection[1] == 0. && this->EndDirection[2] == 0.)
  {
    this->ComputeDirection(this->PointIdList->GetNumberOfIds() - 1, false, this->EndDirection);
  }
  this->ComputeDirection(this->PointIdList->GetNumberOfIds() - 1, false, this->EndDirection);
  return this->EndDirection;
}

double* Segment::GetDirection(vtkIdType pointId, double* direction) const
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

void Segment::ComputeDirection(vtkIdType pointIndex, bool increment, double* direction) const
{
  direction[0] = 0.;
  direction[1] = 0.;
  direction[2] = 0.;

  // Point1
  // vtkIdType pointIndex = this->PointIdList->IsId(pointId);
  vtkIdType pointId = this->PointIdList->GetId(pointIndex);
  if (pointIndex == -1 || pointId == -1)
  {
    cerr << "Given point " << pointId << " doesn't exist." << endl;
    return;
  }
  // cerr<< "DIR: pointId:" << pointId << " pointIndex: " << pointIndex << endl;

  double point1[3];

  this->PolyData->GetPoint(pointId, point1);
  // cerr<< "DIR: point1:" << point1[0] << "," << point1[1] << "," << point1[2] << endl;

  // Point2
  double point2[3];
  pointIndex += increment ? 1 : -1;
  pointId = this->PointIdList->GetId(pointIndex);
  // cerr<< "DIR: pointId:" << pointId << " pointIndex: " << pointIndex << endl;
  if (pointIndex == -1 || pointIndex >= this->PointIdList->GetNumberOfIds())
  { // not enough points to compute the direction
    cerr << " NOT REALLY an error. please erase this line" << pointIndex << endl;
    return;
  }
  this->PolyData->GetPoint(pointId, point2);
  // cerr<< "DIR: point2:" << point2[0] << "," << point2[1] << "," << point2[2] << endl;

  // vector21
  double vector21[3];
  vector21[0] = point1[0] - point2[0];
  vector21[1] = point1[1] - point2[1];
  vector21[2] = point1[2] - point2[2];

  double length = vtkMath::Norm(vector21);
  double averageLength = this->GetLength() / this->PointIdList->GetNumberOfIds();
  while (averageLength > length)
  {
    // cerr<< "DIR: length: " << length << " average:" << averageLength << endl;
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
      cerr << "error. it is not logically possible to get this case." << endl;
      return;
    }
    this->PolyData->GetPoint(pointId, point2);
    // cerr<< "DIR: point1:" << point1[0] << "," << point1[1] << "," << point1[2] << endl;
    // cerr<< "DIR: point2:" << point2[0] << "," << point2[1] << "," << point2[2] << endl;
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
  // cerr<< "DIR: direction:" << direction[0] << "," << direction[1] << "," << direction[2] << endl;
}

void Segment::AddPoint(vtkIdType vtkNotUsed(cellId), vtkIdType pointId)
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
  // double length = vtkMath::Norm(direction);

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

  /*
    cerr <<  __FUNCTION__
    << " pt:" << pointId << "(" << newPoint[0] << "," << newPoint[1]
    << "," << newPoint[2] << ")" << " new length: " << m_Length
    << " startDirection:(" << this->StartDirection[0]
    << "," << this->StartDirection[1] << "," << this->StartDirection[2] << ")"
    << " endDirection:(" << this->EndDirection[0]
    << "," << this->EndDirection[1]
    << "," << this->EndDirection[2] << ")" << endl;
  */
}

void Segment::InsertSegment(vtkIdType pos, Segment* segment)
{
  if (segment->PolyData != this->PolyData)
  {
    cerr << __FUNCTION__ << " can't mix segments with different vtkPolyData." << endl;
    return;
  }
  // GetLength() can change during InsertSegment, we save the result here.
  double length = this->GetLength();
  // cerr << __FUNCTION__ << this->PointIdList->GetNumberOfIds() << " points with "
  //          << segment->this->PointIdList->GetNumberOfIds() << endl;

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
  // cerr << __FUNCTION__ << "startID: " << this->StartId
  //          << " endId: " << this->EndId << endl;
  this->StartDirection[0] = 0.;
  this->StartDirection[1] = 0.;
  this->StartDirection[2] = 0.;
  this->EndDirection[0] = 0.;
  this->EndDirection[1] = 0.;
  this->EndDirection[2] = 0.;

  // cerr << __FUNCTION__ << "end." << endl;
}

vtkStandardNewMacro(Node);

Node::Node()
{
  this->PolyData = NULL;
  this->PointId = -1;
  this->Segments = vtkCollection::New();
}

Node::~Node()
{
  if (this->PolyData)
  {
    this->PolyData->Delete();
  }
  this->Segments->Delete();
}

void Node::AddSegment(Segment* segment)
{
  this->Segments->AddItem(segment);
}

double Node::ComputeConnectionScore(Segment* segment1, Segment* segment2)
{
  if (segment1 == segment2)
  {
    return -1.;
  }
  //  return segment1->GetLength() + segment2->GetLength();

  //        (a.b + 1)     1 -  | ||a||-||b|| |
  // Score =  -------  *(     ----------------- )
  //             2             max(||a||, ||b||)
  // The perfect score(1)is an opposite direction and an
  // identique point frequency
  // add a penalty if the 2 extremities are the same
  double segment1Direction[3], segment2Direction[3];
  segment1->GetDirection(this->PointId, segment1Direction);
  segment2->GetDirection(this->PointId, segment2Direction);
  // cerr << __FUNCTION__ << " normalize" << endl;
  double segment1DirectionNorm = vtkMath::Normalize(segment1Direction);
  double segment2DirectionNorm = vtkMath::Normalize(segment2Direction);

  double angleScore = (1. - vtkMath::Dot(segment1Direction, segment2Direction)) / 2.;
  double pointFrequencyScore = 1. -
    fabs(segment1DirectionNorm - segment2DirectionNorm) /
      MY_MAX(segment1DirectionNorm, segment2DirectionNorm);
  double penaltyScore = 1.;
  // prevent small loops

  double start1[3], end1[3];
  double start2[3], end2[3];
  if (segment1->GetCountPointIds() <= 3 && ((segment1->GetStartId() == segment2->GetStartId() &&
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

vtkStandardNewMacro(vtkPlotEdges);

// Construct object with MaximumLength set to 1000.
vtkPlotEdges::vtkPlotEdges()
{
  this->SetNumberOfOutputPorts(1);
}

vtkPlotEdges::~vtkPlotEdges()
{
}

int vtkPlotEdges::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    return 1;
  }
  return 0;
}

int vtkPlotEdges::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outInfo);

  // The filter accepts vtkPolyData or vtkMultiBlockDataSet as input.
  // The vtkPolyData is trivial
  vtkPolyData* inputPolyData = vtkPolyData::GetData(inInfo);
  if (inputPolyData)
  {
    this->Process(inputPolyData, output);
    return 1;
  }

  // the input multiblock is iterated through and process each vtkPolyData
  // leaf separately
  vtkMultiBlockDataSet* inputMultiBlock = vtkMultiBlockDataSet::GetData(inInfo);
  if (inputMultiBlock)
  {
    // pass the structure to the output. This has to be done on all processes
    // even though actual data is only produce on  the root node.
    output->CopyStructure(inputMultiBlock);

    // iterate over the input multiblock to search for the vtkPolyData leaves
    vtkCompositeDataIterator* it = inputMultiBlock->NewIterator();

    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
      inputPolyData = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
      if (inputPolyData == NULL)
      {
        // this is bad!!! this is assuming that all processes have non-null
        // nodes at the same location, which is true with exodus, but nothing
        // else. Pending BUG #11197.
        continue;
      }

      // A multiblock leaf must be created to be added into the output
      // multiblock. Since the structure on all processes MUST match up, we
      // update the output structure on all processes.
      vtkMultiBlockDataSet* outputMultiBlock = vtkMultiBlockDataSet::New();
      output->SetDataSet(it, outputMultiBlock);
      outputMultiBlock->FastDelete();

      this->Process(inputPolyData, outputMultiBlock);
    }
    it->Delete();
    return 1;
  }

  return 0;
}

void vtkPlotEdges::Process(vtkPolyData* input, vtkMultiBlockDataSet* outputMultiBlock)
{
  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  ReducePolyData(input, inputPolyData);

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  if (controller->GetLocalProcessId() > 0)
  {
    int number_of_blocks = 0;
    controller->Broadcast(&number_of_blocks, 1, 0);
    // to ensure structure matches on all processes.
    outputMultiBlock->SetNumberOfBlocks(number_of_blocks);
    return;
  }

  vtkCollection* segments = vtkCollection::New();
  vtkCollection* nodes = vtkCollection::New();

  this->ExtractSegments(inputPolyData, segments, nodes);
  this->ConnectSegmentsWithNodes(segments, nodes);
  this->SaveToMultiBlockDataSet(segments, outputMultiBlock);

  segments->Delete();
  nodes->Delete();

  int number_of_blocks = outputMultiBlock->GetNumberOfBlocks();
  controller->Broadcast(&number_of_blocks, 1, 0);
}

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

void vtkPlotEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

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

  //   cerr << "Input PolyData nb points:"
  //             << polyData->GetNumberOfPoints() << " nb cells:"
  //             << polyData->GetNumberOfCells() << endl;

  int abort = 0;
  vtkIdType numCells = polyData->GetNumberOfCells();
  vtkIdType progressInterval = numCells / 20 + 1;

  char* visitedCells = new char[numCells];
  memset(visitedCells, 0, numCells * sizeof(char));

  for (vtkIdType cellId = 0; cellId < numCells && !abort; cellId++)
  {
    if (!(cellId % progressInterval))
    { // Give/Get some feedbacks for/from the user
      this->UpdateProgress(static_cast<float>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    if (visitedCells[cellId])
    { // the cell has already been visited, go to the next
      continue;
    }

    if (polyData->GetCellType(cellId) != VTK_LINE && polyData->GetCellType(cellId) != VTK_POLY_LINE)
    { // No other types than VTK_LINE and VTK_POLY_LINE is handled
      continue;
    }

    vtkIdType numCellPts;
    vtkIdType* cellPts;

    // We take the first point from a cell and start to track
    // all the branches from it.
    polyData->GetCellPoints(cellId, numCellPts, cellPts);
    if (numCellPts != 2)
    {
      cerr << "!!! Cell " << cellId << " has " << numCellPts << "pts" << endl;
      continue;
    }

    vtkIdType numPtCells;
    vtkIdType* cellIds;

    // A point can be used by many cells, we get them all
    // and track the branches from each cells
    polyData->GetPointCells(cellPts[0], numPtCells, cellIds);

    // As the point may be in many cells, we need to create a node.
    // The nodes will be merged later in ConnectSegmentsWithNodes
    Node* node = NULL;
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
  visitedCells = NULL;
}

void vtkPlotEdges::ExtractSegmentsFromExtremity(vtkPolyData* polyData, vtkCollection* segments,
  vtkCollection* nodes, char* visitedCells, vtkIdType cellId, vtkIdType pointId, Node* node)
{
  // cerr<< __FUNCTION__ << " cell: " << cellId << " point: " << node->GetPointId() << endl;
  if (visitedCells[cellId])
  {
    // cerr << "Cell already visited: " << cellId << endl;
    // cerr << "End" << __FUNCTION__ << endl;
    return;
  }
  if (polyData->GetCellType(cellId) != VTK_LINE && polyData->GetCellType(cellId) != VTK_POLY_LINE)
  {
    // cerr << "!!!!!!!!!Cell not a line: " << cellId << endl;
    // cerr << "End" << __FUNCTION__ << endl;
    return;
  }

  // vtkIdType pointId = node->GetPointId();

  // Get all the points from the cell
  vtkIdType numCellPts;
  vtkIdType* cellPts;

  polyData->GetCellPoints(cellId, numCellPts, cellPts);

  if (numCellPts != 2)
  {
    cerr << "!!!!!!!The cell " << cellId << " has " << numCellPts << " points" << endl;
    // cerr << "End" << __FUNCTION__ << endl;
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
      // cerr << "At point " << pointId2 << " found a node of "
      //          << numPtCells << " cells " << endl;
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
        // cerr << "At point " << pointId2
        //          << ", get the node branch of cell "
        //          << cellIds[i] << endl;
        if (!visitedCells[cellIds[i]] && (polyData->GetCellType(cellIds[i]) == VTK_LINE ||
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
      // cerr<< "Next Cell is " << cellId2 << endl;
      if (visitedCells[cellId2])
      {
        break;
      }
      if (polyData->GetCellType(cellId2) != VTK_LINE &&
        polyData->GetCellType(cellId2) != VTK_POLY_LINE)
      {
        cerr << "!!!!!! The cell " << cellId2 << " is of type: " << polyData->GetCellType(cellId2)
             << endl;
        // need to do something smarter
        break;
      }
      vtkIdType numCell2Pts;
      vtkIdType* cell2Pts;
      polyData->GetCellPoints(cellId2, numCell2Pts, cell2Pts);

      if (numCell2Pts != 2)
      {
        cerr << "The cell " << cellId << " has " << numCellPts << " points" << endl;
        // need to do something smarter
        break;
      }
      // get the next point
      vtkIdType pointId3 = (cell2Pts[0] == pointId2) ? cell2Pts[1] : cell2Pts[0];
      segment->AddPoint(cellId2, pointId3);

      // go one step forward
      // cerr<< "Next Point is " << pointId3 << endl;
      visitedCells[cellId2] = 1;
      cellId = cellId2;
      pointId2 = pointId3;

      polyData->GetPointCells(pointId2, numPtCells, cellIds);
    }
  }
  // cerr << "End" << __FUNCTION__ << endl;
}

void vtkPlotEdges::ConnectSegmentsWithNodes(vtkCollection* segments, vtkCollection* nodes)
{
  Node* node = NULL;
  // cerr << __FUNCTION__ << ": " << nodes->GetNumberOfItems()
  //          << " nodes." << endl;
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
      // double point[3];
      // node->GetPolyData()->GetPoint(node->GetPointId(), point);
      // cerr << "Node at point " << node->GetPointId()
      //          << "(" << point[0] << "," << point[1] << "," << point[2]
      //          << ") has only 2 segments:(" << segmentA
      //          << ") and(" << segmentB << endl;
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
    // cerr << "Connect node(" << node
    //        << ") at point: " << node->GetPointId()
    //        << "(" << point[0] << ","
    //        << point[1] << "," << point[2] << ")" <<endl;

    while (node->GetSegments()->GetNumberOfItems() > 1)
    {
      // cerr << node->GetSegments()->GetNumberOfItems()
      //           << " segments" << endl;

      vtkCollectionIterator* it = node->GetSegments()->NewIterator();
      vtkCollectionIterator* it2 = node->GetSegments()->NewIterator();

      Segment* segmentI = NULL;
      Segment* segmentJ = NULL;
      Segment* segmentA = NULL;
      Segment* segmentB = NULL;

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

            // cerr << "Score of " << score
            //          << " between segmentI:"<< endl;
            // segmentI->Print(cerr);
            // cerr << "and segmentJ:" << endl;
            // segmentJ->Print(cerr);
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

void vtkPlotEdges::MergeSegments(
  vtkCollection* segments, vtkCollection* nodes, Node* node, Segment* segmentA, Segment* segmentB)
{
  if (segmentA == segmentB)
  {
    node->GetSegments()->RemoveItem(segmentA);
    node->GetSegments()->RemoveItem(segmentB);
    return;
  }

  // cerr << "Merge segment(" << segmentA << ") "
  //             << segmentA->GetStartId() << "-("
  //             << segmentA->GetCountPointIds() << ")-"
  //             << segmentA->GetEndId()
  //             << " with segment(" << segmentB << ") "
  //             << segmentB->GetStartId() << "-("
  //             << segmentB->GetCountPointIds() << ")-"
  //             << segmentB->GetEndId() << endl;
  //   double point[3];
  //   cerr << "Score" << node->ComputeConnectionScore(segmentA, segmentB)
  //             << " Direction 1:" << segmentA->GetDirection(node->GetPointId(),point)[0]
  //             << "," << segmentA->GetDirection(node->GetPointId(),point)[1]
  //             << "," << segmentA->GetDirection(node->GetPointId(),point)[2]
  //             << endl
  //             << " Direction 2:" << segmentB->GetDirection(node->GetPointId(),point)[0]
  //             << "," << segmentB->GetDirection(node->GetPointId(),point)[1]
  //             << "," << segmentB->GetDirection(node->GetPointId(),point)[2]
  //             << endl;

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

Node* vtkPlotEdges::GetNodeAtPoint(vtkCollection* nodes, vtkIdType pointId)
{
  vtkCollectionIterator* it = nodes->NewIterator();
  Node* res = NULL;
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

void vtkPlotEdges::SaveToMultiBlockDataSet(vtkCollection* segments, vtkMultiBlockDataSet* output)
{
  // copy into dataset
  //
  segments->InitTraversal();
  Segment* segment = NULL;
  for (segment = Segment::SafeDownCast(segments->GetNextItemAsObject()); segment;
       segment = Segment::SafeDownCast(segments->GetNextItemAsObject()))
  {
    vtkPolyData* polyData = const_cast<vtkPolyData*>(segment->GetPolyData());

    vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
    output->SetBlock(output->GetNumberOfBlocks(), pd);

    vtkSmartPointer<vtkCellArray> ca = vtkSmartPointer<vtkCellArray>::New();

    vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
    pts->SetDataType(polyData->GetPoints()->GetDataType());

    vtkSmartPointer<vtkIdList> cells = vtkSmartPointer<vtkIdList>::New();

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

    /*
      cerr << "Add PolyLine of " <<
      segment->GetPointIdList()->GetNumberOfIds() << " points" << endl;
      for (vtkIdType i=0; i < segment->GetPointIdList()->GetNumberOfIds(); ++i)
      {
      cerr << segment->GetPointIdList()->GetId(i) << " " ;
      }
      cerr << endl;

      cerr << "ArcLengths " <<
      segment->GetArcLengths()->GetMaxId() << " points" << endl;
      for (vtkIdType i=0; i < segment->GetArcLengths()->GetMaxId(); ++i)
      {
      cerr << segment->GetArcLengths()->GetValue(i) << " " ;
      }
      cerr << endl;
    */
  }
}

void vtkPlotEdges::PrintSegments(vtkCollection* segments)
{
  vtkSmartPointer<vtkCollectionIterator> it = segments->NewIterator();
  for (it->GoToFirstItem(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    Segment* segment = Segment::SafeDownCast(it->GetCurrentObject());
    segment->Print(cerr);
  }
}
