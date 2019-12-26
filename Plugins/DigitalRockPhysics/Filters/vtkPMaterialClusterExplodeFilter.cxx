/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMaterialClusterExplodeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMaterialClusterExplodeFilter.h"

#include "vtkBoundingBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPMaterialClusterAnalysisFilter.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <array>
#include <atomic>
#include <map>

static const int faceIndexes[6][4] = { { 0, 4, 6, 2 }, { 0, 1, 5, 4 }, { 0, 2, 3, 1 },
  { 1, 3, 7, 5 }, { 3, 2, 6, 7 }, { 6, 4, 5, 7 } };

//----------------------------------------------------------------------------
namespace
{
//----------------------------------------------------------------------------
void ComputeStructuredCoordinates(int* extent, int idx, int* ijk)
{
  ijk[0] = extent[0] + idx % (extent[1] - extent[0]);
  ijk[1] = extent[2] + idx / (extent[1] - extent[0]) % (extent[3] - extent[2]);
  ijk[2] = extent[4] + idx / ((extent[1] - extent[0]) * (extent[3] - extent[2]));
}

//----------------------------------------------------------------------------
struct ExplodeParameters
{
  vtkSmartPointer<vtkPoints> Points;
  vtkSmartPointer<vtkCellArray> Cells;
  std::vector<vtkIdType> FacePointIds;
  bool AmIFirstThread;
};

//----------------------------------------------------------------------------
struct ExplodeFunctor
{
  ExplodeFunctor(vtkPMaterialClusterExplodeFilter* filter, vtkImageData* input,
    vtkDataArray* labelArray, double datasetCenter[3],
    std::map<int, std::array<double, 3> >& barycenterMap)
    : Filter(filter)
    , Input(input)
    , LabelArray(labelArray)
    , BarycenterMap(barycenterMap)
  {
    this->ProcessedCells = 0;
    this->Input->GetExtent(this->Extent);
    this->GhostArray = this->Input->GetCellGhostArray();
    this->ExplodeFactor = this->Filter->GetExplodeFactor();
    this->RockfillLabel = this->Filter->GetRockfillLabel();
    std::copy(datasetCenter, datasetCenter + 3, this->DatasetCenter);
  }

  //----------------------------------------------------------------------------
  void Initialize()
  {
    // Initialize thread local object before any processing happens.
    // This gets called once per thread.
    ExplodeParameters& params = this->LocalData.Local();
    params.Points = vtkSmartPointer<vtkPoints>::New();
    params.Cells = vtkSmartPointer<vtkCellArray>::New();
    params.AmIFirstThread = false;
  }

  //----------------------------------------------------------------------------
  void operator()(vtkIdType firstCell, vtkIdType lastCell)
  {
    ExplodeParameters& params = this->LocalData.Local();
    vtkNew<vtkGenericCell> cell;
    vtkPoints* points = params.Points;
    vtkCellArray* cells = params.Cells;
    std::map<std::pair<int, vtkIdType>, vtkIdType> pointIdMap;
    vtkIdType nbCells = lastCell - firstCell;
    vtkIdType progressInterval = nbCells / 1000 + 1;
    vtkIdType cntCells = 0;
    params.FacePointIds.reserve(params.FacePointIds.size() + nbCells);
    if (!params.AmIFirstThread)
    {
      params.AmIFirstThread = (firstCell == 0);
    }
    const double totalNbCells = static_cast<double>(this->Input->GetNumberOfCells());

    // Loop over the points, processing only the one that are needed
    for (vtkIdType cellId = firstCell; cellId < lastCell; cellId++, cntCells++)
    {
      if (!(cntCells % progressInterval) && cntCells > 0)
      {
        this->ProcessedCells += progressInterval;
        if (params.AmIFirstThread)
        {
          this->Filter->UpdateProgress(this->ProcessedCells / totalNbCells);
        }
      }
      if (this->GhostArray && this->GhostArray->GetTuple1(cellId) != 0)
      {
        continue;
      }
      int ijk[3];
      ComputeStructuredCoordinates(this->Extent, cellId, ijk);
      this->Input->GetCell(cellId, cell.Get());
      vtkIdType labelPointId = cell->GetPointId(0);
      int label = this->LabelArray->GetVariantValue(labelPointId).ToInt();
      if (label == this->RockfillLabel)
      {
        continue;
      }
      for (int dir = 0; dir < 3; dir++)
      {
        if (ijk[dir] == this->Extent[dir * 2] || this->CheckOtherCell(ijk, dir, -1, label))
        {
          this->AddFace(dir, cell.Get(), label, pointIdMap, points, cells);
          params.FacePointIds.push_back(labelPointId);
        }
        if (ijk[dir] == this->Extent[dir * 2 + 1] - 1 || this->CheckOtherCell(ijk, dir, 1, label))
        {
          this->AddFace(dir + 3, cell.Get(), label, pointIdMap, points, cells);
          params.FacePointIds.push_back(labelPointId);
        }
      }
    }
  }

  //----------------------------------------------------------------------------
  void AddFace(unsigned int face, vtkCell* cell, int label,
    std::map<std::pair<int, vtkIdType>, vtkIdType>& pointIdMap, vtkPoints* pts,
    vtkCellArray* cells) const
  {
    const int* faceId = faceIndexes[face];
    vtkIdType pointIds[4];
    for (int i = 0; i < 4; i++)
    {
      vtkIdType pointId = cell->GetPointId(faceId[i]);
      auto it = pointIdMap.find({ label, pointId });
      vtkIdType newPointId;
      if (it == pointIdMap.end())
      {
        double p[3];
        this->Input->GetPoint(pointId, p);
        this->TransformPoint(p, this->BarycenterMap[label].data());
        newPointId = pts->InsertNextPoint(p);
        pointIdMap[{ label, pointId }] = newPointId;
      }
      else
      {
        newPointId = it->second;
      }
      pointIds[i] = newPointId;
    }
    cells->InsertNextCell(4, pointIds);
  }

  //----------------------------------------------------------------------------
  bool CheckOtherCell(const int* ijk, int dir, int mod, int label) const
  {
    int ijkTmp[3] = { ijk[0], ijk[1], ijk[2] };
    ijkTmp[dir] += mod;

    // Check for cluster edge
    vtkIdType labelPointId = this->Input->ComputePointId(ijkTmp);
    int otherLabel = this->LabelArray->GetVariantValue(labelPointId).ToInt();
    return label != otherLabel;
  }

  //----------------------------------------------------------------------------
  void TransformPoint(double* point, const double* barycenter) const
  {
    for (int i = 0; i < 3; i++)
    {
      point[i] += this->ExplodeFactor * (barycenter[i] - this->DatasetCenter[i]);
    }
  }

  //----------------------------------------------------------------------------
  void Reduce()
  {
    this->Filter->SetProgressText("Reducing geometry");
    this->Filter->UpdateProgress(0.);

    vtkIdType totalNbPts = 0;
    vtkIdType totalNbCells = 0;
    for (auto params : this->LocalData)
    {
      totalNbPts += params.Points->GetNumberOfPoints();
      totalNbCells += params.Cells->GetNumberOfCells();
    }

    vtkPointData* inPointData = this->Input->GetPointData();
    vtkNew<vtkPoints> outPoints;
    this->Output->SetPoints(outPoints.Get());
    vtkNew<vtkCellArray> outCells;
    this->Output->SetPolys(outCells.Get());
    vtkCellData* outCellData = this->Output->GetCellData();

    outPoints->SetNumberOfPoints(totalNbPts);
    outCells->GetData()->Allocate(totalNbCells * 5);
    outCellData->CopyAllocate(inPointData, totalNbCells);

    vtkIdType newPts[4], cntPts = 0;
    int threadCnt = 0;
    for (auto params : this->LocalData)
    {
      this->Filter->UpdateProgress(threadCnt / static_cast<double>(this->LocalData.size()));

      // Append points
      vtkIdType nbPts = params.Points->GetNumberOfPoints();
      for (vtkIdType i = 0; i < nbPts; i++)
      {
        double p[3];
        params.Points->GetPoint(i, p);
        outPoints->SetPoint(i + cntPts, p);
      }
      // Append cells
      params.Cells->InitTraversal();
      vtkIdType npts;
      const vtkIdType* pts;
      for (vtkIdType i = 0; params.Cells->GetNextCell(npts, pts); i++)
      {
        assert(npts == 4);
        for (vtkIdType j = 0; j < npts; j++)
        {
          newPts[j] = pts[j] + cntPts;
        }
        vtkIdType cid = outCells->InsertNextCell(npts, newPts);
        outCellData->CopyData(inPointData, params.FacePointIds[i], cid);
      }
      cntPts += nbPts;
      threadCnt++;
    }
  }

  //----------------------------------------------------------------------------
  vtkSMPThreadLocal<ExplodeParameters> LocalData;
  std::atomic<vtkIdType> ProcessedCells;
  vtkPMaterialClusterExplodeFilter* Filter;
  vtkImageData* Input;
  vtkDataArray* LabelArray;
  vtkUnsignedCharArray* GhostArray;
  vtkNew<vtkPolyData> Output;
  std::map<int, std::array<double, 3> >& BarycenterMap;
  double DatasetCenter[3];
  double ExplodeFactor;
  int Extent[6];
  int RockfillLabel;
};
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPMaterialClusterExplodeFilter);

//----------------------------------------------------------------------------
vtkPMaterialClusterExplodeFilter::vtkPMaterialClusterExplodeFilter()
{
  this->ExplodeFactor = 1.0;
  this->RockfillLabel = 0;
  this->CacheInput = nullptr;
  this->CacheArray = nullptr;
  this->CacheAnalysis = nullptr;
  this->CacheTime = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
void vtkPMaterialClusterExplodeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Explode Factor: " << this->ExplodeFactor << "\n";
  os << indent << "Rockfill Label: " << this->RockfillLabel << "\n";
}

//----------------------------------------------------------------------------
int vtkPMaterialClusterExplodeFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPMaterialClusterExplodeFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPMaterialClusterExplodeFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);
  vtkDataArray* labelScalarArray = this->GetInputArrayToProcess(0, input);

  if (!input || !output)
  {
    vtkErrorMacro("Missing input or output data.");
    return 0;
  }
  if (!labelScalarArray)
  {
    vtkErrorMacro("Label data array not specified.");
    return 0;
  }

  output->GetFieldData()->ShallowCopy(input->GetFieldData());

  if (this->CacheInput != input || this->CacheArray != labelScalarArray ||
    labelScalarArray->GetMTime() > this->CacheTime || !this->CacheAnalysis)
  {
    this->CacheAnalysis = nullptr;
    this->CacheInput = input;
    this->CacheArray = labelScalarArray;
    this->CacheTime = labelScalarArray->GetMTime();
    vtkAbstractArray* labelArray = input->GetFieldData()->GetAbstractArray("Label");
    vtkDoubleArray* centerArray =
      vtkDoubleArray::SafeDownCast(input->GetFieldData()->GetArray("Center"));
    if (!labelArray || !centerArray)
    {
      this->SetProgressText("Analysing data");

      vtkNew<vtkCallbackCommand> pcbCommand;
      pcbCommand->SetCallback(&vtkPMaterialClusterExplodeFilter::InternalProgressCallbackFunction);
      pcbCommand->SetClientData(this);

      vtkNew<vtkPMaterialClusterAnalysisFilter> analysis;
      analysis->SetInputData(input);
      analysis->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
      analysis->AddObserver(vtkCommand::ProgressEvent, pcbCommand.Get());
      analysis->Update();
      this->CacheAnalysis = analysis->GetOutput();
    }
    else
    {
      this->CacheAnalysis = input;
    }
  }

  vtkDoubleArray* centerArray =
    vtkDoubleArray::SafeDownCast(this->CacheAnalysis->GetFieldData()->GetArray("Center"));
  vtkAbstractArray* labelArray =
    vtkAbstractArray::SafeDownCast(this->CacheAnalysis->GetFieldData()->GetArray("Label"));
  output->GetFieldData()->ShallowCopy(this->CacheAnalysis->GetFieldData());

  if (!labelArray || !centerArray)
  {
    vtkErrorMacro("Unable to perform material analysis.");
    return 0;
  }

  this->SetProgressText("Extracting surfaces");
  this->UpdateProgress(0.01);

  // Compute the whole image center
  int extent[6];
  double *spacing, origin[3], datasetCenter[3];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  spacing = inInfo->Get(vtkDataObject::SPACING());
  inInfo->Get(vtkDataObject::ORIGIN(), origin);
  for (int i = 0; i < 3; i++)
  {
    double dmin = origin[i] + extent[i * 2 + 0] * spacing[i];
    double dmax = origin[i] + extent[i * 2 + 1] * spacing[i];
    datasetCenter[i] = (dmin + dmax) * 0.5;
  }

  // Build the label map
  std::array<double, 3> tmpBarycenter;
  std::map<int, std::array<double, 3> > barycenterMap;
  for (vtkIdType i = 0; i < labelArray->GetNumberOfTuples(); i++)
  {
    centerArray->GetTuple(i, tmpBarycenter.data());
    barycenterMap[labelArray->GetVariantValue(i).ToInt()] = tmpBarycenter;
  }

  // Compute the explode geometry in parallel
  ExplodeFunctor functor(this, this->CacheAnalysis, labelScalarArray, datasetCenter, barycenterMap);
  vtkSMPTools::For(0, this->CacheAnalysis->GetNumberOfCells(), functor);
  vtkPolyData* outPD = functor.Output;

  output->SetPoints(outPD->GetPoints());
  output->SetPolys(outPD->GetPolys());
  output->GetCellData()->ShallowCopy(outPD->GetCellData());

  this->UpdateProgress(1.0);

  return 1;
}

//----------------------------------------------------------------------------
void vtkPMaterialClusterExplodeFilter::InternalProgressCallbackFunction(
  vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eid), void* clientData, void* callData)
{
  vtkPMaterialClusterExplodeFilter* that =
    static_cast<vtkPMaterialClusterExplodeFilter*>(clientData);
  double progress = *static_cast<double*>(callData);
  that->UpdateProgress(progress);
}
