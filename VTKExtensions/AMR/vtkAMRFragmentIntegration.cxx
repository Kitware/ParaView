/*=========================================================================

  Program:   ParaView
  Module:    vtkAMRFragmentIntegration.cxx

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
#include "vtkAMRFragmentIntegration.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkKdTreePointLocator.h"
#include "vtkMultiProcessController.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

#include <map>
#include <vector>
vtkStandardNewMacro(vtkAMRFragmentIntegration);

vtkAMRFragmentIntegration::vtkAMRFragmentIntegration() = default;

vtkAMRFragmentIntegration::~vtkAMRFragmentIntegration() = default;

void vtkAMRFragmentIntegration::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkAMRFragmentIntegration::FillInputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkNonOverlappingAMR");
      break;
    default:
      return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRFragmentIntegration::FillOutputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
    default:
      return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAMRFragmentIntegration::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkErrorMacro("Not yet implemented");
  return 0;
}

//----------------------------------------------------------------------------
vtkTable* vtkAMRFragmentIntegration::DoRequestData(vtkNonOverlappingAMR* volume,
  const char* volumeName, const char* massName, std::vector<std::string> volumeWeightedNames,
  std::vector<std::string> massWeightedNames)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  vtkTable* fragments = vtkTable::New();

  std::map<vtkIdType, vtkIdType> fragIndices;

  vtkTimerLog::MarkStartEvent("Finding max region");

  vtkCompositeDataIterator* iter = volume->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(iter->GetCurrentDataObject());
    if (!grid)
    {
      vtkErrorMacro("NonOverlappingAMR not made up of UniformGrids");
      return nullptr;
    }
    std::string regionName("RegionId-");
    regionName += volumeName;
    vtkDataArray* regionId = grid->GetCellData()->GetArray(regionName.c_str());
    if (!regionId)
    {
      vtkErrorMacro("No RegionID in volume.  Run Connectivity filter.");
      return nullptr;
    }
    vtkUnsignedCharArray* ghostArray = grid->GetCellGhostArray();
    if (!ghostArray)
    {
      vtkErrorMacro("No ghost array attached to the CTH volume data");
      return nullptr;
    }
    for (int c = 0; c < grid->GetNumberOfCells(); c++)
    {
      vtkIdType fragId = static_cast<vtkIdType>(regionId->GetTuple1(c));
      if ((ghostArray->GetValue(c) & vtkDataSetAttributes::DUPLICATECELL) == 0)
      {
        vtkIdType index = fragIndices.size();
        fragIndices[fragId] = index;
      }
    }
  }
  vtkIdType totalFragments = fragIndices.size();
  /*
  if (controller != 0)
    {
    vtkIdType outTotal;
    controller->AllReduce (&totalFragments, &outTotal, 1, vtkCommunicator::SUM_OP);
    totalFragments = outTotal;
    }
    */
  vtkTimerLog::MarkEndEvent("Finding max region");

  vtkTimerLog::MarkStartEvent("Initializing arrays");
  vtkIdTypeArray* fragIdArray = vtkIdTypeArray::New();
  fragIdArray->SetName("Fragment ID");
  fragIdArray->SetNumberOfComponents(1);
  fragIdArray->SetNumberOfTuples(totalFragments + 1);
  fragments->AddColumn(fragIdArray);
  fragIdArray->Delete();

  vtkDoubleArray* fragVolume = vtkDoubleArray::New();
  fragVolume->SetName("Fragment Volume");
  fragVolume->SetNumberOfComponents(1);
  fragVolume->SetNumberOfTuples(totalFragments + 1);
  fragments->AddColumn(fragVolume);
  fragVolume->Delete();

  vtkDoubleArray* fragMass = vtkDoubleArray::New();
  fragMass->SetName("Fragment Mass");
  fragMass->SetNumberOfComponents(1);
  fragMass->SetNumberOfTuples(totalFragments + 1);
  fragments->AddColumn(fragMass);
  fragMass->Delete();

  std::vector<vtkSmartPointer<vtkDoubleArray> > volWeightArrays(volumeWeightedNames.size());
  for (size_t v = 0; v < volumeWeightedNames.size(); v++)
  {
    volWeightArrays[v] = vtkSmartPointer<vtkDoubleArray>::New();
    std::string name("Volume Weighted ");
    name += volumeWeightedNames[v];
    volWeightArrays[v]->SetName(name.c_str());
    volWeightArrays[v]->SetNumberOfComponents(1);
    volWeightArrays[v]->SetNumberOfTuples(totalFragments + 1);
    fragments->AddColumn(volWeightArrays[v]);
  }

  std::vector<vtkSmartPointer<vtkDoubleArray> > massWeightArrays(massWeightedNames.size());
  for (size_t m = 0; m < massWeightedNames.size(); m++)
  {
    massWeightArrays[m] = vtkSmartPointer<vtkDoubleArray>::New();
    std::string name("Mass Weighted ");
    name += massWeightedNames[m];
    massWeightArrays[m]->SetName(name.c_str());
    massWeightArrays[m]->SetNumberOfComponents(1);
    massWeightArrays[m]->SetNumberOfTuples(totalFragments + 1);
    fragments->AddColumn(massWeightArrays[m]);
  }

  for (int i = 0; i < fragIdArray->GetNumberOfTuples(); i++)
  {
    fragIdArray->SetTuple1(i, 0);
    fragVolume->SetTuple1(i, 0);
    fragMass->SetTuple1(i, 0);
    for (size_t v = 0; v < volumeWeightedNames.size(); v++)
    {
      volWeightArrays[v]->SetTuple1(i, 0);
    }
    for (size_t m = 0; m < massWeightedNames.size(); m++)
    {
      massWeightArrays[m]->SetTuple1(i, 0);
    }
  }
  vtkTimerLog::MarkEndEvent("Initializing arrays");

  std::vector<vtkDataArray*> preVolWeightArrays(volumeWeightedNames.size());
  std::vector<vtkDataArray*> preMassWeightArrays(massWeightedNames.size());

  vtkTimerLog::MarkStartEvent("Independent integration");
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(iter->GetCurrentDataObject());
    if (!grid)
    {
      vtkErrorMacro("NonOverlappingAMR not made up of UniformGrids");
      return nullptr;
    }
    vtkUnsignedCharArray* ghostArray = grid->GetCellGhostArray();
    if (!ghostArray)
    {
      vtkErrorMacro("No ghost array attached to the CTH volume data");
      return nullptr;
    }
    std::string regionName("RegionId-");
    regionName += volumeName;
    vtkDataArray* regionId = grid->GetCellData()->GetArray(regionName.c_str());
    if (!regionId)
    {
      vtkErrorMacro("No RegionID in volume.  Run Connectivity filter.");
      return nullptr;
    }
    vtkDataArray* volArray = grid->GetCellData()->GetArray(volumeName);
    if (!volArray)
    {
      vtkErrorMacro(<< "There is no " << volumeName << " in cell field");
      return nullptr;
    }
    vtkDataArray* massArray = grid->GetCellData()->GetArray(massName);
    if (!massArray)
    {
      vtkErrorMacro(<< "There is no " << massName << " in cell field");
      return nullptr;
    }
    for (size_t v = 0; v < volumeWeightedNames.size(); v++)
    {
      preVolWeightArrays[v] = grid->GetCellData()->GetArray(volumeWeightedNames[v].c_str());
    }
    for (size_t m = 0; m < massWeightedNames.size(); m++)
    {
      preMassWeightArrays[m] = grid->GetCellData()->GetArray(massWeightedNames[m].c_str());
    }
    double* spacing = grid->GetSpacing();
    double cellVol = spacing[0] * spacing[1] * spacing[2];
    for (int c = 0; c < grid->GetNumberOfCells(); c++)
    {
      if (regionId->GetTuple1(c) > 0.0 &&
        (ghostArray->GetValue(c) & vtkDataSetAttributes::DUPLICATECELL) == 0)
      {
        vtkIdType fragId = static_cast<vtkIdType>(regionId->GetTuple1(c));
        std::map<vtkIdType, vtkIdType>::iterator loc = fragIndices.find(fragId);
        if (loc == fragIndices.end())
        {
          vtkErrorMacro(<< "Invalid Region Id " << fragId);
        }
        int index = fragIndices[fragId];
        fragIdArray->SetTuple1(index, fragId);
        double vol = volArray->GetTuple1(c) * cellVol / 255.0;
        fragVolume->SetTuple1(index, fragVolume->GetTuple1(index) + vol);
        double mass = massArray->GetTuple1(c);
        fragMass->SetTuple1(index, fragMass->GetTuple1(index) + mass);

        for (size_t v = 0; v < volumeWeightedNames.size(); v++)
        {
          double value = volWeightArrays[v]->GetTuple1(index);
          value += preVolWeightArrays[v]->GetTuple1(c) * vol;
          volWeightArrays[v]->SetTuple1(index, value);
        }
        for (size_t m = 0; m < massWeightedNames.size(); m++)
        {
          double value = massWeightArrays[m]->GetTuple1(index);
          value += preMassWeightArrays[m]->GetTuple1(c) * mass;
          massWeightArrays[m]->SetTuple1(index, value);
        }
      }
    }
  }
  vtkTimerLog::MarkEndEvent("Independent integration");

  vtkTimerLog::MarkStartEvent("Combining integration");

  int myProc = 0;
  int numProcs = 1;
  if (controller != nullptr)
  {
    myProc = controller->GetLocalProcessId();
    numProcs = controller->GetNumberOfProcesses();

    vtkIdTypeArray* fragIndicesReceive = vtkIdTypeArray::New();
    fragIndicesReceive->SetNumberOfComponents(2);

    vtkDoubleArray* fragVolumeReceive = vtkDoubleArray::New();
    fragVolumeReceive->SetNumberOfComponents(1);
    // fragVolumeReceive->SetNumberOfTuples (totalFragments + 1);

    vtkDoubleArray* fragMassReceive = vtkDoubleArray::New();
    fragMassReceive->SetNumberOfComponents(1);
    // fragMassReceive->SetNumberOfTuples (totalFragments + 1);

    std::vector<vtkSmartPointer<vtkDoubleArray> > volWeightReceive(volumeWeightedNames.size());
    for (size_t v = 0; v < volumeWeightedNames.size(); v++)
    {
      volWeightReceive[v] = vtkSmartPointer<vtkDoubleArray>::New();
      volWeightReceive[v]->SetNumberOfComponents(1);
      // volWeightReceive[v]->SetNumberOfTuples (totalFragments + 1);
    }

    std::vector<vtkSmartPointer<vtkDoubleArray> > massWeightReceive(massWeightedNames.size());
    for (size_t m = 0; m < massWeightedNames.size(); m++)
    {
      massWeightReceive[m] = vtkDoubleArray::New();
      massWeightReceive[m]->SetNumberOfComponents(1);
      // massWeightReceive[m]->SetNumberOfTuples (totalFragments + 1);
    }

    int tag = 728574;
    int pivot = (numProcs > 1 ? (numProcs + 1) / 2 : 0);
    while (pivot > 0 && myProc < (pivot * 2))
    {
      if (myProc >= pivot)
      {
        int tuples = static_cast<int>(fragIndices.size());
        vtkIdTypeArray* fragIndicesArray = vtkIdTypeArray::New();
        fragIndicesArray->SetNumberOfComponents(2);
        fragIndicesArray->SetNumberOfTuples(tuples);

        std::map<vtkIdType, vtkIdType>::iterator mapiter;
        int ind = 0;
        for (mapiter = fragIndices.begin(); mapiter != fragIndices.end(); mapiter++)
        {
          fragIndicesArray->SetComponent(ind, 0, mapiter->first);
          fragIndicesArray->SetComponent(ind, 1, mapiter->second);
          ind++;
        }

        controller->Send(&tuples, 1, myProc - pivot, tag + pivot + 0);
        controller->Send(fragIndicesArray, myProc - pivot, tag + pivot + 1);
        controller->Send(fragVolume, myProc - pivot, tag + pivot + 2);
        controller->Send(fragMass, myProc - pivot, tag + pivot + 3);

        int off = 4;
        for (size_t v = 0; v < volumeWeightedNames.size(); v++)
        {
          controller->Send(volWeightArrays[v], myProc - pivot, tag + pivot + off);
          off++;
        }

        for (size_t m = 0; m < massWeightedNames.size(); m++)
        {
          controller->Send(massWeightArrays[m], myProc - pivot, tag + pivot + off);
          off++;
        }

        fragIndicesArray->Delete();
      }
      else if ((myProc + pivot) < numProcs)
      {
        int tuples = 0;
        controller->Receive(&tuples, 1, myProc + pivot, tag + pivot + 0);
        fragIndicesReceive->SetNumberOfTuples(tuples);
        controller->Receive(fragIndicesReceive, myProc + pivot, tag + pivot + 1);
        fragVolumeReceive->SetNumberOfTuples(tuples + 1);
        controller->Receive(fragVolumeReceive, myProc + pivot, tag + pivot + 2);
        fragMassReceive->SetNumberOfTuples(tuples + 1);
        controller->Receive(fragMassReceive, myProc + pivot, tag + pivot + 3);

        int off = 4;
        for (size_t v = 0; v < volumeWeightedNames.size(); v++)
        {
          volWeightReceive[v]->SetNumberOfTuples(tuples + 1);
          controller->Receive(volWeightReceive[v], myProc + pivot, tag + pivot + off);
          off++;
        }

        for (size_t m = 0; m < massWeightedNames.size(); m++)
        {
          massWeightReceive[m]->SetNumberOfTuples(tuples + 1);
          controller->Receive(massWeightReceive[m], myProc + pivot, tag + pivot + off);
          off++;
        }

        for (int i = 0; i < fragIndicesReceive->GetNumberOfTuples(); i++)
        {
          vtkIdType fragId = fragIndicesReceive->GetComponent(i, 0);
          vtkIdType remoteIndex = fragIndicesReceive->GetComponent(i, 1);
          std::map<vtkIdType, vtkIdType>::iterator loc = fragIndices.find(fragId);
          if (loc == fragIndices.end())
          {
            size_t index = fragIndices.size();
            fragIndices[fragId] = index;
            fragIdArray->InsertNextTuple1(fragId);
            fragVolume->InsertNextTuple(remoteIndex, fragVolumeReceive);
            fragMass->InsertNextTuple(remoteIndex, fragMassReceive);
            for (size_t v = 0; v < volumeWeightedNames.size(); v++)
            {
              volWeightArrays[v]->InsertNextTuple(remoteIndex, volWeightReceive[v]);
            }
            for (size_t m = 0; m < massWeightedNames.size(); m++)
            {
              massWeightArrays[m]->InsertNextTuple(remoteIndex, massWeightReceive[m]);
            }
          }
          else
          {
            int index = fragIndices[fragId];
            fragIdArray->SetTuple1(index, fragId);
            fragVolume->SetTuple1(
              index, fragVolume->GetTuple1(index) + fragVolumeReceive->GetTuple1(remoteIndex));
            fragMass->SetTuple1(
              index, fragMass->GetTuple1(index) + fragMassReceive->GetTuple1(remoteIndex));
            for (size_t v = 0; v < volumeWeightedNames.size(); v++)
            {
              volWeightArrays[v]->SetTuple1(index,
                volWeightArrays[v]->GetTuple1(index) + volWeightReceive[v]->GetTuple1(remoteIndex));
            }
            for (size_t m = 0; m < massWeightedNames.size(); m++)
            {
              massWeightArrays[m]->SetTuple1(index, massWeightArrays[m]->GetTuple1(index) +
                  massWeightReceive[m]->GetTuple1(remoteIndex));
            }
          }
        }
      }
      pivot /= 2;
    }

    fragIndicesReceive->Delete();
    fragVolumeReceive->Delete();
    fragMassReceive->Delete();
  }

  if (myProc == 0)
  {
    int row = 0;
    while (row < fragments->GetNumberOfRows())
    {
      if (fragIdArray->GetValue(row) == 0)
      {
        fragments->RemoveRow(row);
      }
      else
      {
        for (size_t v = 0; v < volumeWeightedNames.size(); v++)
        {
          double weightAvg = volWeightArrays[v]->GetTuple1(row) / fragVolume->GetTuple1(row);
          volWeightArrays[v]->SetTuple1(row, weightAvg);
        }
        for (size_t m = 0; m < massWeightedNames.size(); m++)
        {
          double weightAvg = massWeightArrays[m]->GetTuple1(row) / fragMass->GetTuple1(row);
          massWeightArrays[m]->SetTuple1(row, weightAvg);
        }

        row++;
      }
    }
  }
  else
  {
    fragments->SetNumberOfRows(0);
  }
  vtkTimerLog::MarkEndEvent("Combining integration");
  return fragments;
}
