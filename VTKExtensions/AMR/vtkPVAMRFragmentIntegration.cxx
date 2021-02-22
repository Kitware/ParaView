/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAMRFragmentIntegration.cxx

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  Copyright 2013 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.

=========================================================================*/
#include "vtkPVAMRFragmentIntegration.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"

#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkTable.h"

#include <string> // STL required.
#include <vector> // STL required.

vtkStandardNewMacro(vtkPVAMRFragmentIntegration);

//-----------------------------------------------------------------------------
class vtkPVAMRFragmentIntegrationInternal
{
public:
  std::vector<std::string> VolumeArrays;
  std::vector<std::string> MassArrays;
  std::vector<std::string> VolumeWeightedArrays;
  std::vector<std::string> MassWeightedArrays;
};

//-----------------------------------------------------------------------------
vtkPVAMRFragmentIntegration::vtkPVAMRFragmentIntegration()
{
  this->Implementation = new vtkPVAMRFragmentIntegrationInternal();
}

//-----------------------------------------------------------------------------
vtkPVAMRFragmentIntegration::~vtkPVAMRFragmentIntegration()
{
  if (this->Implementation)
  {
    delete this->Implementation;
    this->Implementation = nullptr;
  }
}

//-----------------------------------------------------------------------------
void vtkPVAMRFragmentIntegration::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPVAMRFragmentIntegration::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkNonOverlappingAMR* amrInput =
    vtkNonOverlappingAMR::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* outInfo;
  outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* mbdsOutput0 =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  unsigned int noOfArrays = static_cast<unsigned int>(this->Implementation->VolumeArrays.size());
  for (unsigned int i = 0; i < noOfArrays; i++)
  {
    vtkTable* out = this->DoRequestData(amrInput, this->Implementation->VolumeArrays[i].c_str(),
      this->Implementation->MassArrays[i].c_str(), this->Implementation->VolumeWeightedArrays,
      this->Implementation->MassWeightedArrays);

    if (out)
    {
      /// Assign the name to the block by the array name.
      mbdsOutput0->SetBlock(i, out);
      out->Delete();
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVAMRFragmentIntegration::AddInputVolumeArrayToProcess(const char* name)
{
  this->Implementation->VolumeArrays.push_back(std::string(name));
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAMRFragmentIntegration::ClearInputVolumeArrayToProcess()
{
  this->Implementation->VolumeArrays.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAMRFragmentIntegration::AddInputMassArrayToProcess(const char* name)
{
  this->Implementation->MassArrays.push_back(std::string(name));
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAMRFragmentIntegration::ClearInputMassArrayToProcess()
{
  this->Implementation->MassArrays.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAMRFragmentIntegration::AddInputVolumeWeightedArrayToProcess(const char* name)
{
  this->Implementation->VolumeWeightedArrays.push_back(std::string(name));
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAMRFragmentIntegration::ClearInputVolumeWeightedArrayToProcess()
{
  this->Implementation->VolumeWeightedArrays.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAMRFragmentIntegration::AddInputMassWeightedArrayToProcess(const char* name)
{
  this->Implementation->MassWeightedArrays.push_back(std::string(name));
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAMRFragmentIntegration::ClearInputMassWeightedArrayToProcess()
{
  this->Implementation->MassWeightedArrays.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAMRFragmentIntegration::SetContourConnection(vtkAlgorithmOutput* output)
{
  this->SetInputConnection(1, output);
}
