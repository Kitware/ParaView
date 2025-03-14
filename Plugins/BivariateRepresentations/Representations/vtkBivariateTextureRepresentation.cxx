// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBivariateTextureRepresentation.h"

#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkView.h"

vtkStandardNewMacro(vtkBivariateTextureRepresentation);

//----------------------------------------------------------------------------
vtkBivariateTextureRepresentation::vtkBivariateTextureRepresentation() = default;

//----------------------------------------------------------------------------
vtkBivariateTextureRepresentation::~vtkBivariateTextureRepresentation() = default;

//----------------------------------------------------------------------------
void vtkBivariateTextureRepresentation::SetTexture(vtkTexture* texture)
{
  this->Superclass::SetTexture(texture);
  this->MarkModified(); // To force a new RequestData pass
}

//----------------------------------------------------------------------------
int vtkBivariateTextureRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
  {
    return false;
  }

  vtkDataObjectTree* inputDOT = vtkDataObjectTree::SafeDownCast(this->GetRenderedDataObject(0));
  if (!inputDOT)
  {
    // Rendered data object is always composite. If null, we do not render anything yet.
    return true;
  }

  // Retrieve input array names
  vtkInformation* info = this->GetInputArrayInformation(0);
  std::string arrayName1 =
    info->Has(vtkDataObject::FIELD_NAME()) ? info->Get(vtkDataObject::FIELD_NAME()) : "";

  info = this->GetInputArrayInformation(1);
  std::string arrayName2 =
    info->Has(vtkDataObject::FIELD_NAME()) ? info->Get(vtkDataObject::FIELD_NAME()) : "";

  auto iter = vtk::TakeSmartPointer(inputDOT->NewTreeIterator());
  iter->SkipEmptyNodesOn();
  iter->VisitOnlyLeavesOn();

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkDataSet* inputDS = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (!inputDS)
    {
      // This can happen in client/server mode : RequestData is called on both
      // processes but data is only retrieved on server.
      return true;
    }

    // Ensure we don't keep previous tcoords in case following conditions are not met
    inputDS->GetAttributes(vtkDataObject::POINT)->SetTCoords(nullptr);

    // Retrieve input arrays
    auto* array1 =
      vtkDataArray::SafeDownCast(inputDS->GetPointData()->GetAbstractArray(arrayName1.c_str()));
    auto* array2 =
      vtkDataArray::SafeDownCast(inputDS->GetPointData()->GetAbstractArray(arrayName2.c_str()));

    if (!array1 || !array2)
    {
      // info->Get(vtkDataObject::FIELD_NAME()) can return non-empty
      // array names even if no array is available (e.g. "1 0 0 0 0")
      // In such case, we just call superclass RequestData
      return true;
    }

    if (array1->GetNumberOfComponents() != 1 || array2->GetNumberOfComponents() != 1)
    {
      vtkWarningMacro(
        "First and second arrays should only have one component to generate TCoords.");
      return true;
    }

    // Compute texture coordinates from input array
    this->TCoordsArray->Initialize();
    this->TCoordsArray->SetName("BivariateTCoords");
    this->TCoordsArray->SetNumberOfComponents(2);
    this->TCoordsArray->SetNumberOfTuples(array1->GetNumberOfTuples());

    // Update arrays name & ranges
    auto* range1 = array1->GetRange();
    this->FirstArrayRange[0] = range1[0];
    this->FirstArrayRange[1] = range1[1];
    this->FirstArrayName = arrayName1;

    auto* range2 = array2->GetRange();
    this->SecondArrayRange[0] = range2[0];
    this->SecondArrayRange[1] = range2[1];
    this->SecondArrayName = arrayName2;

    for (int i = 0; i < this->TCoordsArray->GetNumberOfTuples(); i++)
    {
      auto firstValue = (array1->GetTuple1(i) - range1[0]) / (range1[1] - range1[0]);
      auto secondValue = (array2->GetTuple1(i) - range2[0]) / (range2[1] - range2[0]);
      this->TCoordsArray->SetTuple2(i, firstValue, secondValue);
    }

    inputDS->GetAttributes(vtkDataObject::POINT)->SetTCoords(this->TCoordsArray);
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkBivariateTextureRepresentation::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* attributeTypeorName)
{
  if ((idx == 0 || idx == 1) &&
    fieldAssociation == vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_POINTS)
  {
    this->MarkModified(); // To force a new RequestData pass
  }
  this->Superclass::SetInputArrayToProcess(
    idx, port, connection, fieldAssociation, attributeTypeorName);
}

//----------------------------------------------------------------------------
void vtkBivariateTextureRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->TCoordsArray)
  {
    os << indent << "TCoordsArray:" << endl;
    this->TCoordsArray->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "TCoordsArray: (None)" << endl;
  }

  os << indent << "FirstArrayRange: [" << this->FirstArrayRange[0] << ", "
     << this->FirstArrayRange[1] << "]" << endl;
  os << indent << "SecondArrayRange: [" << this->SecondArrayRange[0] << ", "
     << this->SecondArrayRange[1] << "]" << endl;
  os << indent << "FirstArrayName: " << this->FirstArrayName << endl;
  os << indent << "SecondArrayName: " << this->SecondArrayName << endl;
}
