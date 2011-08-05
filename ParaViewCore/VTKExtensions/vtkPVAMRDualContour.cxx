/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAMRDualContour.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAMRDualContour.h"

#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"

#include "vtkCompositeDataIterator.h"

#include <vtkstd/string>  // STL required.
#include <vtkstd/vector>  // STL required.

vtkStandardNewMacro(vtkPVAMRDualContour);

const double PV_AMR_SURFACE_VALUE_UNSIGNED_CHAR=255;

//-----------------------------------------------------------------------------
class vtkPVAMRDualContourInternal
{
public:
  vtkstd::vector<vtkstd::string> CellArrays;
};

//-----------------------------------------------------------------------------
vtkPVAMRDualContour::vtkPVAMRDualContour() :
  VolumeFractionSurfaceValue(1.0)
{
  this->Implementation = new vtkPVAMRDualContourInternal();
}

//-----------------------------------------------------------------------------
vtkPVAMRDualContour::~vtkPVAMRDualContour()
{
  if(this->Implementation)
    {
    delete this->Implementation;
    this->Implementation = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkPVAMRDualContour::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPVAMRDualContour::RequestData(vtkInformation* vtkNotUsed(request),
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkHierarchicalBoxDataSet* hbdsInput=vtkHierarchicalBoxDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo;
  outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* mbdsOutput0 = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));


  // Don't call SetIsoValue() that changes filter's MTime which is not
  // acceptable in RequestData().
  this->IsoValue = (this->VolumeFractionSurfaceValue *
                    PV_AMR_SURFACE_VALUE_UNSIGNED_CHAR);

  size_t noOfArrays = this->Implementation->CellArrays.size();
  for(size_t i = 0; i < noOfArrays; i++)
    {
    vtkMultiBlockDataSet* out = this->DoRequestData(
      hbdsInput, this->Implementation->CellArrays[i].c_str());

    if(out)
      {
      /// Assign the name to the block by the array name.
      mbdsOutput0->SetBlock(i, out);
      out->Delete();
      }
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVAMRDualContour::AddInputCellArrayToProcess(const char* name)
{
  this->Implementation->CellArrays.push_back(vtkstd::string(name));
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkPVAMRDualContour::ClearInputCellArrayToProcess()
{
  this->Implementation->CellArrays.clear();
  this->Modified();
}
