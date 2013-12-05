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
#include "vtkUniformGrid.h"

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
                                        vtkInformationVector** inputVector,
                                        vtkInformationVector* outputVector)
{
  vtkErrorMacro ("Not yet implemented");
  return 0;
}

//----------------------------------------------------------------------------
vtkTable* vtkAMRFragmentIntegration::DoRequestData(vtkNonOverlappingAMR* volume, 
                                  const char* volumeName,
                                  const char* massName)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController ();

  vtkTable* fragments = vtkTable::New ();

  vtkIdType maxRegion = 0;
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
      if (fragId > maxRegion &&  ghostLevels->GetTuple1 (c) < 0.5)
        {
        maxRegion = fragId;
        }
      }
    }

  if (controller != 0)
    {
    controller->AllReduce (&maxRegion, &maxRegion, 1, vtkCommunicator::MAX_OP);
    }

  vtkIdTypeArray* fragIdArray = vtkIdTypeArray::New ();
  fragIdArray->SetName ("Fragment ID");
  fragIdArray->SetNumberOfComponents (1);
  fragIdArray->SetNumberOfTuples (maxRegion + 1);
  fragments->AddColumn (fragIdArray);
  fragIdArray->Delete ();

  vtkDoubleArray* fragVolume = vtkDoubleArray::New ();
  fragVolume->SetName ("Fragment Volume");
  fragVolume->SetNumberOfComponents (1);
  fragVolume->SetNumberOfTuples (maxRegion + 1);
  fragments->AddColumn (fragVolume);
  fragVolume->Delete ();

  vtkDoubleArray* fragMass = vtkDoubleArray::New ();
  fragMass->SetName ("Fragment Mass");
  fragMass->SetNumberOfComponents (1);
  fragMass->SetNumberOfTuples (maxRegion + 1);
  fragments->AddColumn (fragMass);
  fragMass->Delete ();

  for (int i = 0; i < fragIdArray->GetNumberOfTuples (); i ++) 
    {
    fragIdArray->SetTuple1 (i, 0);
    fragVolume->SetTuple1 (i, 0);
    fragMass->SetTuple1 (i, 0);
    }

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
        fragIdArray->SetTuple1 (fragId, fragId);
        fragVolume->SetTuple1 (fragId, 
            fragVolume->GetTuple1 (fragId) + volArray->GetTuple1 (c) * cellVol / 255.0);
        fragMass->SetTuple1 (fragId, fragMass->GetTuple1 (fragId) + massArray->GetTuple1 (c));
        }
      }
    }

  if (controller != 0)
    {
    int myProc = controller->GetLocalProcessId ();

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
    else 
      {
      // it's all on process 0 now.
      fragments->SetNumberOfRows (0);
      }
    }

  return fragments;
}
