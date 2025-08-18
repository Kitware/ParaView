// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBivariateTextureRepresentation.h"

#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

namespace
{
//----------------------------------------------------------------------------
void InitializeRangeIfUnset(double range[2], const double actualRange[2])
{
  if (range[0] == 0.0 && range[1] == 0.0)
  {
    range[0] = actualRange[0];
    range[1] = actualRange[1];
  }
}

//----------------------------------------------------------------------------
double ComputeDenominator(const double range[2])
{
  const double denom = range[1] - range[0];
  const double epsilon = 1e-6;
  return denom < epsilon ? epsilon : denom;
}

//----------------------------------------------------------------------------
void FillTCoordsFromArrays(vtkDataArray* array1, const double range1[2], vtkDataArray* array2,
  const double range2[2], vtkDataArray* tcoords)
{
  const double denom1 = ComputeDenominator(range1);
  const double denom2 = ComputeDenominator(range2);

  for (vtkIdType i = 0; i < tcoords->GetNumberOfTuples(); ++i)
  {
    const double firstValue = (array1->GetTuple1(i) - range1[0]) / denom1;
    const double secondValue = (array2->GetTuple1(i) - range2[0]) / denom2;
    tcoords->SetTuple2(i, firstValue, secondValue);
  }
}
}

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
void vtkBivariateTextureRepresentation::SetArrayRange(double range[2], double min, double max)
{
  if (min > max)
  {
    std::swap(min, max);
  }
  range[0] = min;
  range[1] = max;
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkBivariateTextureRepresentation::SetXArrayRange(double min, double max)
{
  this->SetArrayRange(this->XArrayRange, min, max);
}

//----------------------------------------------------------------------------
void vtkBivariateTextureRepresentation::SetYArrayRange(double min, double max)
{
  this->SetArrayRange(this->YArrayRange, min, max);
}

//----------------------------------------------------------------------------
int vtkBivariateTextureRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
  {
    return false;
  }

  if (this->Mapper)
  {
    this->Mapper->SetScalarVisibility(false);
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
    auto* range2 = array2->GetRange();

    ::InitializeRangeIfUnset(this->XArrayRange, range1);
    ::InitializeRangeIfUnset(this->YArrayRange, range2);

    this->XArrayName = arrayName1;
    this->YArrayName = arrayName2;

    ::FillTCoordsFromArrays(
      array1, this->XArrayRange, array2, this->YArrayRange, this->TCoordsArray);

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

  os << indent << "XArrayRange: [" << this->XArrayRange[0] << ", " << this->XArrayRange[1] << "]"
     << endl;
  os << indent << "YArrayRange: [" << this->YArrayRange[0] << ", " << this->YArrayRange[1] << "]"
     << endl;
  os << indent << "XArrayName: " << this->XArrayName << endl;
  os << indent << "YArrayName: " << this->YArrayName << endl;
}
