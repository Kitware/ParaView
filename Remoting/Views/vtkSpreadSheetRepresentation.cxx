/*=========================================================================

  Program:   ParaView
  Module:    vtkSpreadSheetRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpreadSheetRepresentation.h"

#include "vtkBlockDeliveryPreprocessor.h"
#include "vtkCleanArrays.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSpreadSheetView.h"

vtkStandardNewMacro(vtkSpreadSheetRepresentation);
//----------------------------------------------------------------------------
vtkSpreadSheetRepresentation::vtkSpreadSheetRepresentation()
{
  this->SetNumberOfInputPorts(3);
  this->DataConditioner->SetGenerateOriginalIds(1);
  this->DataConditioner->SetFlattenTable(true);
  this->DataConditioner->SetSplitComponentsNamingMode(
    vtkSplitColumnComponents::NUMBERS_WITH_UNDERSCORES);
  this->CleanArrays->SetInputConnection(this->DataConditioner->GetOutputPort());

  this->ExtractedDataConditioner->SetGenerateOriginalIds(0);
  this->ExtractedDataConditioner->SetFlattenTable(true);
  this->ExtractedDataConditioner->SetSplitComponentsNamingMode(
    vtkSplitColumnComponents::NUMBERS_WITH_UNDERSCORES);
  this->ExtractedCleanArrays->SetInputConnection(this->ExtractedDataConditioner->GetOutputPort());
}

//----------------------------------------------------------------------------
vtkSpreadSheetRepresentation::~vtkSpreadSheetRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkSpreadSheetRepresentation::SetFieldAssociation(int val)
{
  if (val != this->DataConditioner->GetFieldAssociation())
  {
    this->DataConditioner->SetFieldAssociation(val);
    this->ExtractedDataConditioner->SetFieldAssociation(val);
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkSpreadSheetRepresentation::AddCompositeDataSetIndex(unsigned int val)
{
  this->DataConditioner->AddCompositeDataSetIndex(val);
  this->ExtractedDataConditioner->AddCompositeDataSetIndex(val);
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkSpreadSheetRepresentation::RemoveAllCompositeDataSetIndices()
{
  this->DataConditioner->RemoveAllCompositeDataSetIndices();
  this->ExtractedDataConditioner->RemoveAllCompositeDataSetIndices();
  this->MarkModified();
}

//----------------------------------------------------------------------------
int vtkSpreadSheetRepresentation::FillInputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    case 0:
    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
      break;

    case 2:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
      break;

    default:
      return 0;
  }

  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSpreadSheetRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (this->GetVisibility() == false)
  {
    return 0;
  }

  if (request == vtkPVView::REQUEST_UPDATE())
  {
    if (vtkSpreadSheetView* view = vtkSpreadSheetView::SafeDownCast(inInfo->Get(vtkPVView::VIEW())))
    {
      this->SetGenerateCellConnectivity(view->GetGenerateCellConnectivity());
      this->SetFieldAssociation(view->GetFieldAssociation());
    }
  }

  return this->Superclass::ProcessViewRequest(request, inInfo, outInfo);
}

//----------------------------------------------------------------------------
int vtkSpreadSheetRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (vtkProcessModule::GetProcessType() == vtkProcessModule::PROCESS_RENDER_SERVER)
  {
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  this->DataConditioner->RemoveAllInputs();
  this->ExtractedDataConditioner->RemoveAllInputs();

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    this->DataConditioner->SetInputConnection(this->GetInternalOutputPort(0, 0));
  }
  if (inputVector[1]->GetNumberOfInformationObjects() == 1)
  {
    this->ExtractedDataConditioner->SetInputConnection(this->GetInternalOutputPort(1, 0));
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkSpreadSheetRepresentation::GetDataProducer()
{
  return this->DataConditioner->GetNumberOfInputConnections(0) == 1
    ? this->CleanArrays->GetOutputPort(0)
    : NULL;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkSpreadSheetRepresentation::GetExtractedDataProducer()
{
  return this->ExtractedDataConditioner->GetNumberOfInputConnections(0) == 1
    ? this->ExtractedCleanArrays->GetOutputPort(0)
    : NULL;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkSpreadSheetRepresentation::GetSelectionProducer()
{
  if (this->GetNumberOfInputConnections(1) == 1)
  {
    return this->GetInternalOutputPort(1, 0);
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSpreadSheetRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSpreadSheetRepresentation::SetGenerateCellConnectivity(bool v)
{
  if (this->DataConditioner->GetGenerateCellConnectivity() != v)
  {
    this->DataConditioner->SetGenerateCellConnectivity(v);
    this->ExtractedDataConditioner->SetGenerateCellConnectivity(v);
    this->MarkModified();
  }
}
