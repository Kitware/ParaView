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

#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkCommunicator.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkKdTreePointLocator.h"
#include "vtkMultiProcessController.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkUniformGrid.h"

#include <map>

vtkStandardNewMacro(vtkAMRFragmentIntegration);

vtkAMRFragmentIntegration::vtkAMRFragmentIntegration ()
{
}

vtkAMRFragmentIntegration::~vtkAMRFragmentIntegration()
{
}

void vtkAMRFragmentIntegration::PrintSelf (ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkAMRFragmentIntegration::FillInputPortInformation(int port,
                                                vtkInformation *info)
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
int vtkAMRFragmentIntegration::FillOutputPortInformation(int port, vtkInformation *info)
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
                                        vtkInformationVector** vtkNotUsed(inputVector),
                                        vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkErrorMacro ("Not yet implemented");
  return 0;
}

//----------------------------------------------------------------------------
vtkTable* vtkAMRFragmentIntegration::DoRequestData(vtkNonOverlappingAMR* volume, 
                                  const char* volumeName,
                                  const char* massName,
                                  std::vector<std::string> volumeWeightedNames,
                                  std::vector<std::string> massWeightedNames)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController ();

  vtkTable* fragments = vtkTable::New ();

  
  std::map<vtkIdType, int> fragIndices;

  vtkTimerLog::MarkStartEvent ("Finding max region");

  vtkCompositeDataIterator* iter = volume->NewIterator ();
  for (iter->InitTraversal (); !iter->IsDoneWithTraversal (); iter->GoToNextItem ())
    {
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast (iter->GetCurrentDataObject ());
    if (!grid) 
      {
      vtkErrorMacro ("NonOverlappingAMR not made up of UniformGrids");
      return 0;
      }
    std::string regionName ("RegionId-");
    regionName += volumeName;
    vtkDataArray* regionId = grid->GetCellData ()->GetArray (regionName.c_str());
    if (!regionId) 
      {
      vtkErrorMacro ("No RegionID in volume.  Run Connectivity filter.");
      return 0;
      }
    vtkDataArray* ghostLevels = grid->GetCellData ()->GetArray ("vtkGhostLevels");
    if (!ghostLevels) 
      {
      vtkErrorMacro ("No vtkGhostLevels array attached to the CTH volume data");
      return 0;
      }
    for (int c = 0; c < grid->GetNumberOfCells (); c ++)
      {
      vtkIdType fragId = static_cast<vtkIdType> (regionId->GetTuple1 (c));
      if (ghostLevels->GetTuple1 (c) < 0.5)
        {
        int index = fragIndices.size ();
        fragIndices[fragId] = index;
        }
      }
    }
  int totalFragments = fragIndices.size ();
  if (controller != 0)
    {
    controller->AllReduce (&totalFragments, &totalFragments, 1, vtkCommunicator::MAX_OP);
    }
  vtkTimerLog::MarkEndEvent ("Finding max region");

  vtkTimerLog::MarkStartEvent ("Initializing arrays");
  vtkIdTypeArray* fragIdArray = vtkIdTypeArray::New ();
  fragIdArray->SetName ("Fragment ID");
  fragIdArray->SetNumberOfComponents (1);
  fragIdArray->SetNumberOfTuples (totalFragments + 1);
  fragments->AddColumn (fragIdArray);
  fragIdArray->Delete ();

  vtkDoubleArray* fragVolume = vtkDoubleArray::New ();
  fragVolume->SetName ("Fragment Volume");
  fragVolume->SetNumberOfComponents (1);
  fragVolume->SetNumberOfTuples (totalFragments + 1);
  fragments->AddColumn (fragVolume);
  fragVolume->Delete ();

  vtkDoubleArray* fragMass = vtkDoubleArray::New ();
  fragMass->SetName ("Fragment Mass");
  fragMass->SetNumberOfComponents (1);
  fragMass->SetNumberOfTuples (totalFragments + 1);
  fragments->AddColumn (fragMass);
  fragMass->Delete ();

  vtkDoubleArray** volWeightArrays = new vtkDoubleArray*[volumeWeightedNames.size ()];
  for (int v = 0; v < volumeWeightedNames.size (); v ++)
    {
    volWeightArrays[v] = vtkDoubleArray::New ();
    std::string name ("Volume Weighted ");
    name += volumeWeightedNames[v];
    volWeightArrays[v]->SetName (name.c_str ());
    volWeightArrays[v]->SetNumberOfComponents (1);
    volWeightArrays[v]->SetNumberOfTuples (totalFragments + 1);
    fragments->AddColumn (volWeightArrays[v]);
    volWeightArrays[v]->Delete ();
    }

  vtkDoubleArray** massWeightArrays = new vtkDoubleArray*[massWeightedNames.size ()];
  for (int m = 0; m < massWeightedNames.size (); m ++)
    {
    massWeightArrays[m] = vtkDoubleArray::New ();
    std::string name ("Mass Weighted ");
    name += massWeightedNames[m];
    massWeightArrays[m]->SetName (name.c_str ());
    massWeightArrays[m]->SetNumberOfComponents (1);
    massWeightArrays[m]->SetNumberOfTuples (totalFragments + 1);
    fragments->AddColumn (massWeightArrays[m]);
    massWeightArrays[m]->Delete ();
    }

  for (int i = 0; i < fragIdArray->GetNumberOfTuples (); i ++) 
    {
    fragIdArray->SetTuple1 (i, 0);
    fragVolume->SetTuple1 (i, 0);
    fragMass->SetTuple1 (i, 0);
    for (int v = 0; v < volumeWeightedNames.size (); v ++)
      {
      volWeightArrays[v]->SetTuple1 (i, 0);
      }
    for (int m = 0; m < massWeightedNames.size (); m ++)
      {
      massWeightArrays[m]->SetTuple1 (i, 0);
      }
    }
  vtkTimerLog::MarkEndEvent ("Initializing arrays");

  vtkDataArray** preVolWeightArrays = new vtkDataArray*[volumeWeightedNames.size ()];
  vtkDataArray** preMassWeightArrays = new vtkDataArray*[massWeightedNames.size ()];

  vtkTimerLog::MarkStartEvent ("Independent integration");
  for (iter->InitTraversal (); !iter->IsDoneWithTraversal (); iter->GoToNextItem ())
    {
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast (iter->GetCurrentDataObject ());
    if (!grid) 
      {
      vtkErrorMacro ("NonOverlappingAMR not made up of UniformGrids");
      return 0;
      }
    vtkDataArray* ghostLevels = grid->GetCellData ()->GetArray ("vtkGhostLevels");
    if (!ghostLevels) 
      {
      vtkErrorMacro ("No vtkGhostLevels array attached to the CTH volume data");
      return 0;
      }
    std::string regionName ("RegionId-");
    regionName += volumeName;
    vtkDataArray* regionId = grid->GetCellData ()->GetArray (regionName.c_str());
    if (!regionId) 
      {
      vtkErrorMacro ("No RegionID in volume.  Run Connectivity filter.");
      return 0;
      }
    vtkDataArray* volArray = grid->GetCellData ()->GetArray (volumeName);
    if (!volArray)
      {
      vtkErrorMacro (<< "There is no " << volumeName << " in cell field");
      return 0;
      }
    vtkDataArray* massArray = grid->GetCellData ()->GetArray (massName);
    if (!massArray)
      {
      vtkErrorMacro (<< "There is no " << massName << " in cell field");
      return 0;
      }
    for (int v = 0; v < volumeWeightedNames.size (); v ++)
      {
      preVolWeightArrays[v] = grid->GetCellData ()->GetArray (volumeWeightedNames[v].c_str ());
      }
    for (int m = 0; m < massWeightedNames.size (); m ++)
      {
      preMassWeightArrays[m] = grid->GetCellData ()->GetArray (massWeightedNames[m].c_str ());
      }
    double* spacing = grid->GetSpacing ();
    double cellVol = spacing[0] * spacing[1] * spacing[2];
    for (int c = 0; c < grid->GetNumberOfCells (); c ++)
      {
      if (regionId->GetTuple1 (c) > 0.0 && ghostLevels->GetTuple1 (c) < 0.5) 
        {
        vtkIdType fragId = static_cast<vtkIdType> (regionId->GetTuple1 (c));
        if (fragId > fragIdArray->GetNumberOfTuples ()) 
          {
          vtkErrorMacro (<< "Invalid Region Id " << fragId);
          }
        int index = fragIndices[fragId];
        fragIdArray->SetTuple1 (index, fragId);
        double vol = volArray->GetTuple1 (c) * cellVol / 255.0;
        fragVolume->SetTuple1 (index, fragVolume->GetTuple1 (index) + vol);
        double mass = massArray->GetTuple1 (c);
        fragMass->SetTuple1 (index, fragMass->GetTuple1 (index) + mass);

        for (int v = 0; v < volumeWeightedNames.size (); v ++)
          {
          double value = volWeightArrays[v]->GetTuple1 (index);
          value += preVolWeightArrays[v]->GetTuple1 (c) * vol;
          volWeightArrays[v]->SetTuple1 (index, value);
          }
        for (int m = 0; m < massWeightedNames.size (); m ++)
          {
          double value = massWeightArrays[m]->GetTuple1 (index);
          value += preMassWeightArrays[m]->GetTuple1 (c) * mass;
          massWeightArrays[m]->SetTuple1 (index, value);
          }
        }
      }
    }
  vtkTimerLog::MarkEndEvent ("Independent integration");

  vtkTimerLog::MarkStartEvent ("Combining integration");

  int myProc = 0;
  int numProcs = 1;
  if (controller != 0)
    {
    myProc = controller->GetLocalProcessId ();
    numProcs = controller->GetNumberOfProcesses ();

/*
    vtkIdTypeArray *copyFragId;
    vtkDoubleArray *copyFragVolume;
    vtkDoubleArray *copyFragMass;
    if (myProc == 0)
      {
      copyFragId = vtkIdTypeArray::New ();
      copyFragId->DeepCopy (fragIdArray);
      copyFragVolume = vtkDoubleArray::New ();
      copyFragVolume->DeepCopy (fragVolume);
      copyFragMass = vtkDoubleArray::New ();
      copyFragMass->DeepCopy (fragMass);
      }
    else
      {
      copyFragId = fragIdArray;
      copyFragVolume = fragVolume;
      copyFragMass = fragMass;
      }
    
    controller->Reduce (copyFragId, fragIdArray, vtkCommunicator::MAX_OP, 0);
    controller->Reduce (copyFragVolume, fragVolume, vtkCommunicator::SUM_OP, 0);
    controller->Reduce (copyFragMass, fragMass, vtkCommunicator::SUM_OP, 0);

    if (myProc == 0)
      {
      copyFragId->Delete ();
      copyFragVolume->Delete ();
      copyFragMass->Delete ();
      }

    for (int v = 0; v < volumeWeightedNames.size (); v ++) 
      {
      vtkDoubleArray *copyVolWeight;
      if (myProc == 0)
        {
        copyVolWeight = vtkDoubleArray::New ();
        copyVolWeight->DeepCopy (volWeightArrays[v]);
        }
      else
        {
        copyVolWeight = volWeightArrays[v];
        }
      controller->Reduce (copyVolWeight, volWeightArrays[v], vtkCommunicator::SUM_OP, 0);

      if (myProc == 0)
        {
        copyVolWeight->Delete ();
        }
      }

    for (int m = 0; m < massWeightedNames.size (); m ++) 
      {
      vtkDoubleArray *copyMassWeight;
      if (myProc == 0)
        {
        copyMassWeight = vtkDoubleArray::New ();
        copyMassWeight->DeepCopy (massWeightArrays[m]);
        }
      else
        {
        copyMassWeight = massWeightArrays[m];
        }
      controller->Reduce (copyMassWeight, massWeightArrays[m], vtkCommunicator::SUM_OP, 0);
      if (myProc == 0)
        {
        copyMassWeight->Delete ();
        }
      }

    else 
      {
      // it's all on process 0 now.
      fragments->SetNumberOfRows (0);
      }
*/
    }

  if (myProc == 0)
    {
    int row = 0;
    while (row < fragments->GetNumberOfRows ())
      {
      if (fragIdArray->GetValue (row) == 0)
        {
        fragments->RemoveRow (row);
        }
      else
        {
        for (int v = 0; v < volumeWeightedNames.size (); v ++) 
          {
          double weightAvg = volWeightArrays[v]->GetTuple1 (row) / fragVolume->GetTuple1 (row);
          volWeightArrays[v]->SetTuple1 (row, weightAvg);
          }
        for (int m = 0; m < massWeightedNames.size (); m ++) 
          {
          double weightAvg = massWeightArrays[m]->GetTuple1 (row) / fragMass->GetTuple1 (row);
          massWeightArrays[m]->SetTuple1 (row, weightAvg);
          }
        
        row ++;
        }
      }
    }
  vtkTimerLog::MarkEndEvent ("Combining integration");

  delete [] volWeightArrays;
  delete [] massWeightArrays;

  return fragments;
}
