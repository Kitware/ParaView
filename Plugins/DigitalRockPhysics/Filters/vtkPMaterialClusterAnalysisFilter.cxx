/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMaterialClusterAnalysisFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMaterialClusterAnalysisFilter.h"

#include "vtkAtomicTypes.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkTable.h"

#define BROADCAST_VALUES_TAG 621

#include <array>
#include <map>
#include <set>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPMaterialClusterAnalysisFilter);

//----------------------------------------------------------------------------
vtkPMaterialClusterAnalysisFilter::vtkPMaterialClusterAnalysisFilter()
{
  this->RockfillLabel = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  this->SetNumberOfOutputPorts(2);
}

//----------------------------------------------------------------------------
void vtkPMaterialClusterAnalysisFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Rockfill Label: " << this->RockfillLabel << "\n";
}

//----------------------------------------------------------------------------
namespace
{
typedef std::map<int, std::pair<unsigned int, std::array<double, 3> > >
  LabelValuesMap;                                    // cluster label -> { count, barycenter }
typedef std::map<int, std::set<int> > LabelRanksMap; // cluster label -> { list of rank using it }

//----------------------------------------------------------------------------
void Barycenter(unsigned int weight1, const double* point1, unsigned int weight2,
  const double* point2, double* barycenter)
{
  double div = weight1 + weight2;
  for (int i = 0; i < 3; i++)
  {
    barycenter[i] = (point1[i] * weight1 + point2[i] * weight2) / div;
  }
}

//----------------------------------------------------------------------------
void AppendMapToTable(LabelValuesMap& lvMap, vtkTable* table)
{
  vtkNew<vtkIntArray> labelArray;
  labelArray->SetName("Label");
  labelArray->SetNumberOfTuples(lvMap.size());

  vtkNew<vtkDoubleArray> volumeArray;
  volumeArray->SetName("Volume");
  volumeArray->SetNumberOfTuples(lvMap.size());

  vtkNew<vtkDoubleArray> barycenterArray;
  barycenterArray->SetName("Center");
  barycenterArray->SetNumberOfComponents(3);
  barycenterArray->SetNumberOfTuples(lvMap.size());

  vtkIdType idx = 0;
  for (auto it = lvMap.begin(); it != lvMap.end(); ++it, ++idx)
  {
    labelArray->SetValue(idx, it->first);
    volumeArray->SetValue(idx, it->second.first);
    barycenterArray->SetTuple(idx, it->second.second.data());
  }

  table->AddColumn(labelArray.Get());
  table->AddColumn(volumeArray.Get());
  table->AddColumn(barycenterArray.Get());
}

//----------------------------------------------------------------------------
bool AppendTableToMap(vtkTable* table, LabelValuesMap& lvMap, int rockfillLabel, bool center = true)
{
  vtkIntArray* labelArray = vtkIntArray::SafeDownCast(table->GetColumnByName("Label"));
  vtkDoubleArray* volumeArray = vtkDoubleArray::SafeDownCast(table->GetColumnByName("Volume"));
  vtkDoubleArray* barycenterArray = vtkDoubleArray::SafeDownCast(table->GetColumnByName("Center"));

  if (!labelArray || !volumeArray || (!barycenterArray && center))
  {
    vtkErrorWithObjectMacro(table, "Could not Append table to map");
    return false;
  }

  std::array<double, 3> barycenter;
  vtkIdType nRows = table->GetNumberOfRows();
  for (vtkIdType iRow = 0; iRow < nRows; iRow++)
  {
    int label = labelArray->GetValue(iRow);
    if (label != rockfillLabel)
    {
      double volume = volumeArray->GetValue(iRow);
      if (center)
      {
        barycenterArray->GetTuple(iRow, &barycenter[0]);
      }
      auto iter = lvMap.emplace(label, std::make_pair(volume, barycenter));
      if (!iter.second)
      {
        if (center)
        {
          Barycenter(iter.first->second.first, &iter.first->second.second[0], volume,
            &barycenter[0], &iter.first->second.second[0]);
        }
        iter.first->second.first += volume;
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
int ReduceTable(vtkAlgorithm* that, LabelValuesMap& lvMap, vtkTable* table, int rockfillLabel)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if (!controller || (controller && controller->GetNumberOfProcesses() <= 1))
  {
    return 1;
  }

  that->SetProgressText("Reducing data");
  that->UpdateProgress(0.0);
  std::vector<vtkSmartPointer<vtkDataObject> > recv;
  controller->Gather(table, recv, 0);
  while (table->GetNumberOfColumns() != 0)
  {
    table->RemoveColumn(0);
  }

  if (controller->GetLocalProcessId() == 0)
  {
    LabelRanksMap ranksMap;
    for (size_t iRank = 1; iRank < recv.size(); iRank++)
    {
      vtkTable* localTable = vtkTable::SafeDownCast(recv[iRank]);
      if (!localTable)
      {
        vtkErrorWithObjectMacro(controller, "Could not reduce tables from other ranks");
        return 0;
      }

      // Create Label Values Map
      if (!AppendTableToMap(localTable, lvMap, rockfillLabel))
      {
        vtkErrorWithObjectMacro(
          controller, "Could not append table to map. This rank will be ignored.");
        continue;
      }

      // Create ranks map
      vtkIdType nRows = localTable->GetNumberOfRows();
      vtkIntArray* tmpLabelArray = vtkIntArray::SafeDownCast(localTable->GetColumnByName("Label"));
      if (!tmpLabelArray)
      {
        vtkErrorWithObjectMacro(
          controller, "Could not create ranks map. This rank will be ignored.");
        continue;
      }
      for (vtkIdType iRow = 0; iRow < nRows; iRow++)
      {
        int tmpLabel = tmpLabelArray->GetValue(iRow);
        ranksMap[tmpLabel].insert(static_cast<int>(iRank));
      }
    }
    that->UpdateProgress(0.8);

    // Export map to a vtkTable
    AppendMapToTable(lvMap, table);

    // Broadcast it rank by rank using the ranks map
    vtkDataSetAttributes* attr = table->GetRowData();
    vtkIntArray* labelArray = vtkIntArray::SafeDownCast(table->GetColumnByName("Label"));
    for (int iRank = 1; iRank < controller->GetNumberOfProcesses(); iRank++)
    {
      vtkNew<vtkTable> localTable;
      vtkDataSetAttributes* localAttr = localTable->GetRowData();
      localAttr->CopyAllocate(attr);
      vtkIdType iRowLocal = 0;
      for (vtkIdType iRow = 0; iRow < table->GetNumberOfRows(); iRow++)
      {
        auto tmpSet = ranksMap[labelArray->GetValue(iRow)];
        if (tmpSet.find(iRank) != tmpSet.end())
        {
          localAttr->CopyData(attr, iRow, iRowLocal);
          iRowLocal++;
        }
      }
      localAttr->Squeeze();
      controller->Send(localTable, iRank, BROADCAST_VALUES_TAG);
    }
  }
  else
  {
    // Receive our own label values map
    controller->Receive(table, 0, BROADCAST_VALUES_TAG);
  }

  return 1;
}

//----------------------------------------------------------------------------
struct vtkLocalDataType
{
  LabelValuesMap LabelMap;
};

//----------------------------------------------------------------------------
struct AnalysisFunctor
{
  AnalysisFunctor(
    vtkPMaterialClusterAnalysisFilter* filter, vtkImageData* input, vtkDataArray* array)
    : Filter(filter)
    , Input(input)
    , Array(array)
  {
    this->ProcessedPoints = 0;
  }

  //----------------------------------------------------------------------------
  void Initialize() { this->AmIFirstThread.Local() = 0; }

  //----------------------------------------------------------------------------
  void operator()(vtkIdType firstPoint, vtkIdType lastPoint)
  {
    // Loop over the points, processing only the one that are needed
    LabelValuesMap& lvMap = this->LocalData.Local();
    std::array<double, 3> voxelPoint;
    int& amITheFirstThread = this->AmIFirstThread.Local();
    if (amITheFirstThread == 0)
    {
      amITheFirstThread = (firstPoint == 0) ? 1 : 0;
    }
    int rockfillLabel = this->Filter->GetRockfillLabel();
    vtkIdType progressInterval = (lastPoint - firstPoint) / 1000 + 1;
    const double totalNbPoints = static_cast<double>(this->Input->GetNumberOfPoints());
    for (vtkIdType pointId = firstPoint, cntPoints = 0; pointId < lastPoint; pointId++, cntPoints++)
    {
      if (!(cntPoints % progressInterval) && cntPoints > 0)
      {
        this->ProcessedPoints += progressInterval;
        if (amITheFirstThread == 1)
        {
          this->Filter->UpdateProgress(this->ProcessedPoints / totalNbPoints);
        }
      }
      int tmpLabel = this->Array->GetVariantValue(pointId).ToInt();
      if (tmpLabel != rockfillLabel)
      {
        this->Input->GetPoint(pointId, &voxelPoint[0]);
        auto iter = lvMap.emplace(tmpLabel, std::make_pair(1, voxelPoint));
        if (!iter.second)
        {
          Barycenter(iter.first->second.first, &iter.first->second.second[0], 1, &voxelPoint[0],
            &iter.first->second.second[0]);
          iter.first->second.first++;
        }
      }
    }
  }

  //----------------------------------------------------------------------------
  void Reduce()
  {
    this->Filter->SetProgressText("Reducing geometry");
    this->Filter->UpdateProgress(0.);
    vtkSMPThreadLocal<LabelValuesMap>::iterator outIter = this->LocalData.begin();
    this->OutputLabelMap = *outIter;
    outIter++;
    for (int threadCnt = 0; outIter != this->LocalData.end(); ++outIter, ++threadCnt)
    {
      this->Filter->UpdateProgress(threadCnt / static_cast<double>(this->LocalData.size()));

      for (auto it : *outIter)
      {
        auto iter = this->OutputLabelMap.emplace(it.first, it.second);
        if (!iter.second)
        {
          Barycenter(iter.first->second.first, &iter.first->second.second[0], it.second.first,
            &it.second.second[0], &iter.first->second.second[0]);
          iter.first->second.first += it.second.first;
        }
      }
    }
  }

  //----------------------------------------------------------------------------
  vtkSMPThreadLocal<LabelValuesMap> LocalData;
  vtkSMPThreadLocal<int> AmIFirstThread;
  vtkPMaterialClusterAnalysisFilter* Filter;
  vtkImageData* Input;
  vtkDataArray* Array;
  vtkAtomicIdType ProcessedPoints;
  LabelValuesMap OutputLabelMap;
};
}

//----------------------------------------------------------------------------
int vtkPMaterialClusterAnalysisFilter::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  }
  else
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkTable");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPMaterialClusterAnalysisFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
  vtkImageData* output = vtkImageData::GetData(outputVector, 0);
  vtkTable* outputTable = vtkTable::GetData(outputVector, 1);
  vtkDataArray* array = this->GetInputArrayToProcess(0, input);

  output->ShallowCopy(input);

  this->SetProgressText("Processing data");
  this->UpdateProgress(0.01);

  vtkIdType nPoints = input->GetNumberOfPoints();

  AnalysisFunctor functor(this, input, array);
  vtkSMPTools::For(0, nPoints, functor);
  LabelValuesMap& lvMap = functor.OutputLabelMap;

  this->SetProgressText("Processing data");
  this->UpdateProgress(0.0);

  vtkNew<vtkTable> table;
  ::AppendMapToTable(lvMap, table.Get());

  if (!::ReduceTable(this, lvMap, table.Get(), this->RockfillLabel))
  {
    return 0;
  }

  // Copy reduced volume to map
  LabelValuesMap localLvMap;
  ::AppendTableToMap(table, localLvMap, this->RockfillLabel, false);

  // Use map to fill point data with volume and center
  vtkNew<vtkDoubleArray> volumeArray;
  volumeArray->SetName("Volume");
  volumeArray->SetNumberOfTuples(array->GetNumberOfTuples());
  for (vtkIdType i = 0; i < array->GetNumberOfTuples(); i++)
  {
    int tmpLabel = array->GetVariantValue(i).ToInt();
    if (tmpLabel != this->RockfillLabel)
    {
      auto& value = localLvMap[tmpLabel];
      volumeArray->SetValue(i, value.first);
    }
    else
    {
      volumeArray->SetValue(i, 0.);
    }
  }

  vtkPointData* outputPd = output->GetPointData();
  outputPd->AddArray(volumeArray.Get());

  // Copy table columns as data fields
  vtkIdType nbCols = table->GetNumberOfColumns();
  for (vtkIdType i = 0; i < nbCols; i++)
  {
    output->GetFieldData()->AddArray(table->GetColumn(i));
  }

  outputTable->AddColumn(table->GetColumnByName("Volume"));
  outputTable->AddColumn(table->GetColumnByName("Label"));

  this->UpdateProgress(1.0);

  return 1;
}
