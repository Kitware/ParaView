/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMetaClipDataSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMetaClipDataSet.h"

#include "vtkAlgorithm.h"
#include "vtkDataObject.h"
#include "vtkExtractGeometry.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVClipDataSet.h"
#include "vtkPVPlane.h"
#include "vtkSmartPointer.h"

class vtkPVMetaClipDataSet::vtkInternals
{
public:
  vtkNew<vtkPVClipDataSet> Clip;
  vtkNew<vtkExtractGeometry> ExtractCells;

  vtkInternals()
  {
    this->ExtractCells->SetExtractInside(1);
    this->ExtractCells->SetExtractOnlyBoundaryCells(0);
    this->ExtractCells->SetExtractBoundaryCells(1);
  }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMetaClipDataSet);

//----------------------------------------------------------------------------
vtkPVMetaClipDataSet::vtkPVMetaClipDataSet()
  : ExactBoxClip(false)
{
  // Setup default configuration
  this->SetOutputType(VTK_UNSTRUCTURED_GRID);

  this->Internal = new vtkInternals();

  this->ImplicitFunctions[METACLIP_DATASET] = nullptr;
  this->ImplicitFunctions[METACLIP_HYPERTREEGRID] = nullptr;

  this->RegisterFilter(this->Internal->Clip.GetPointer());
  this->RegisterFilter(this->Internal->ExtractCells.GetPointer());

  this->Superclass::SetActiveFilter(0);
}

//----------------------------------------------------------------------------
vtkPVMetaClipDataSet::~vtkPVMetaClipDataSet()
{
  delete this->Internal;
  this->Internal = nullptr;
}

//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ExactBoxClip: " << this->ExactBoxClip << endl;
}

//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::SetImplicitFunction(vtkImplicitFunction* func)
{
  this->Internal->Clip->SetClipFunction(func);
  this->Internal->ExtractCells->SetImplicitFunction(func);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::SetDataSetClipFunction(vtkImplicitFunction* func)
{
  this->ImplicitFunctions[METACLIP_DATASET] = func;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::SetHyperTreeGridClipFunction(vtkImplicitFunction* func)
{
  this->ImplicitFunctions[METACLIP_HYPERTREEGRID] = func;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::SetValue(double value)
{
  this->Internal->Clip->SetValue(value);
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::SetUseValueAsOffset(int value)
{
  this->Internal->Clip->SetUseValueAsOffset(value != 0);
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::PreserveInputCells(int keepCellAsIs)
{
  if (this->Internal->Clip->GetClipFunction() == this->ImplicitFunctions[METACLIP_DATASET])
  {
    this->SetActiveFilter(keepCellAsIs);
  }
}
//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Internal->Clip->SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);
  this->Modified();
}
//----------------------------------------------------------------------------

void vtkPVMetaClipDataSet::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, int fieldAttributeType)
{
  this->Internal->Clip->SetInputArrayToProcess(
    idx, port, connection, fieldAssociation, fieldAttributeType);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::SetInputArrayToProcess(int idx, vtkInformation* info)
{
  this->Internal->Clip->SetInputArrayToProcess(idx, info);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::SetInputArrayToProcess(
  int idx, int port, int connection, const char* fieldName, const char* fieldType)
{
  this->Internal->Clip->SetInputArrayToProcess(idx, port, connection, fieldName, fieldType);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVMetaClipDataSet::SetInsideOut(int insideOut)
{
  this->Internal->Clip->SetInsideOut(insideOut);
  this->Internal->ExtractCells->SetExtractInside(insideOut);
  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkPVMetaClipDataSet::SwitchFilterForCrinkle()
{
  if (!this->Internal->ExtractCells->GetImplicitFunction() &&
    this->GetActiveFilter() == this->Internal->ExtractCells.GetPointer())
  {
    // We can not use vtkExtractGeometry without the ImplicitFunction being set
    this->PreserveInputCells(0);
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
int vtkPVMetaClipDataSet::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::GetData(inputVector[0], 0);
  bool needSwitch = htg ? false : this->SwitchFilterForCrinkle();
  this->Internal->Clip->SetExactBoxClip(this->ExactBoxClip);
  int res = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  if (needSwitch)
  {
    this->PreserveInputCells(1);
  }
  return res;
}

//----------------------------------------------------------------------------
int vtkPVMetaClipDataSet::ProcessRequest(
  vtkInformation* request, vtkCollection* inputVector, vtkInformationVector* outputVector)
{
  bool needSwitch = this->SwitchFilterForCrinkle();
  this->Internal->Clip->SetExactBoxClip(this->ExactBoxClip);
  int res = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  if (needSwitch)
  {
    this->PreserveInputCells(1);
  }
  return res;
}

//----------------------------------------------------------------------------
int vtkPVMetaClipDataSet::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!inputVector && !inputVector[0])
  {
    return 0;
  }

  if (vtkHyperTreeGrid::SafeDownCast(
        inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT())))
  {
    this->SetOutputType(VTK_HYPER_TREE_GRID);
    this->Internal->Clip->SetClipFunction(this->ImplicitFunctions[METACLIP_HYPERTREEGRID]);
    this->Internal->ExtractCells->SetImplicitFunction(
      this->ImplicitFunctions[METACLIP_HYPERTREEGRID]);
  }
  else
  {
    this->SetOutputType(VTK_UNSTRUCTURED_GRID);
    this->Internal->Clip->SetClipFunction(this->ImplicitFunctions[METACLIP_DATASET]);
    this->Internal->ExtractCells->SetImplicitFunction(this->ImplicitFunctions[METACLIP_DATASET]);
  }

  return this->Superclass::RequestDataObject(request, inputVector, outputVector);
}
