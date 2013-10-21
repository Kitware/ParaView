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
#include "vtkCompositeDataIterator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkTable.h"
#include "vtkUniformGrid.h"

vtkStandardNewMacro(vtkAMRFragmentIntegration);

vtkAMRFragmentIntegration::vtkAMRFragmentIntegration ()
{
  this->SetNumberOfInputPorts (2);
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
    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
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
                                  vtkDataSet* contour,
                                  const char* volumeName,
                                  const char* massName)
{
  vtkTable* fragments = vtkTable::New ();

  vtkDoubleArray* fragVolume = vtkDoubleArray::New ();
  fragVolume->SetName ("Fragment Volume");
  fragVolume->SetNumberOfComponents (1);
  fragVolume->SetNumberOfTuples (0);
  fragments->AddColumn (fragVolume);
  fragVolume->Delete ();

  vtkDoubleArray* fragMass = vtkDoubleArray::New ();
  fragMass->SetName ("Fragment Mass");
  fragMass->SetNumberOfComponents (1);
  fragMass->SetNumberOfTuples (0);
  fragments->AddColumn (fragMass);
  fragMass->Delete ();

  vtkDataArray* regionId = contour->GetPointData ()->GetArray ("RegionId");
  if (!regionId) 
    {
    vtkErrorMacro ("No RegionID in contour.  Run Connectivity filter.");
    return 0;
    }

  vtkCompositeDataIterator* iter = volume->NewIterator ();

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
      if (volArray->GetTuple1 (c) > 0.0 && ghostLevels->GetTuple1 (c) < 0.5) 
        {
        double* bounds = grid->GetCell (c)->GetBounds ();
        
        // This is the critical piece.
        // Because we've run the contour and we're operating this function on 
        // one material contour at a time, we can expect an air gap of at least 
        // one cell between fragments.  So we just need to find a point in the 
        // nearest fragment contour to this cell and get its region ID.  This
        // tells us the regionID for this cell as computed by the AMRDualContour 
        // and Connectivity filters.
        vtkIdType pointId = contour->FindPoint (bounds[0], bounds[2], bounds[4]);
        vtkIdType fragId = static_cast<vtkIdType> (regionId->GetTuple1 (pointId));

        while (fragments->GetNumberOfRows () <= fragId) 
          {
          fragments->InsertNextBlankRow ();
          }
        fragVolume->SetTuple1 (fragId, fragVolume->GetTuple1 (fragId) + volArray->GetTuple1 (c) * cellVol / 255.0);
        fragMass->SetTuple1 (fragId, fragMass->GetTuple1 (fragId) + massArray->GetTuple1 (c));
        }
      }
    }

  return fragments;
}
