/*=========================================================================

  Program:   ParaView
  Module:    vtkSteeringDataGenerator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSteeringDataGenerator.h"

#include "vtkDataObjectTypes.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"

#include <array>
#include <cassert>
#include <map>
#include <string>

class vtkSteeringDataGenerator::vtkInternals
{
public:
  std::map<std::string, vtkSmartPointer<vtkDataArray> > Arrays;

  template <typename VTKArrayType, int NumComponents,
    typename ValueType = typename VTKArrayType::ValueType>
  void SetTuple(const std::string& arrayname, vtkIdType index,
    const std::array<ValueType, NumComponents>& tuple)
  {
    VTKArrayType* vtkarray = nullptr;
    auto iter = this->Arrays.find(arrayname);
    if (iter == this->Arrays.end())
    {
      vtkarray = VTKArrayType::New();
      vtkarray->SetNumberOfComponents(NumComponents);
      vtkarray->SetName(arrayname.c_str());
      this->Arrays[arrayname].TakeReference(vtkarray);
    }
    else
    {
      vtkarray = VTKArrayType::SafeDownCast(iter->second.GetPointer());
      assert(vtkarray != nullptr);
    }
    assert(vtkarray->GetNumberOfComponents() == NumComponents);

    if (vtkarray->GetNumberOfTuples() < (index + 1))
    {
      vtkarray->Resize(512 * (static_cast<vtkIdType>(index + 512) / 512));
      vtkarray->SetNumberOfTuples(index + 1);
    }
    vtkarray->SetTypedTuple(index, tuple.data());
  }

  bool Validate()
  {
    // ensure all arrays have same number of tuples.
    const vtkIdType numTuples =
      this->Arrays.size() > 0 ? this->Arrays.begin()->second->GetNumberOfTuples() : 0;
    for (const auto& pair : this->Arrays)
    {
      if (pair.second->GetNumberOfTuples() != numTuples)
      {
        vtkLogF(ERROR, "Incorrect number of tuples for array '%s' expected %d, got %d",
          pair.first.c_str(), static_cast<int>(numTuples),
          static_cast<int>(pair.second->GetNumberOfTuples()));
        return false;
      }
    }
    return true;
  }
};

vtkStandardNewMacro(vtkSteeringDataGenerator);
//----------------------------------------------------------------------------
vtkSteeringDataGenerator::vtkSteeringDataGenerator()
  : PartitionType{ VTK_TABLE }
  , FieldAssociation{ vtkDataObject::FIELD_ASSOCIATION_ROWS }
  , Internals(new vtkSteeringDataGenerator::vtkInternals())
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkSteeringDataGenerator::~vtkSteeringDataGenerator()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
int vtkSteeringDataGenerator::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkSteeringDataGenerator::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return this->Superclass::RequestInformation(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkSteeringDataGenerator::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector);
  output->Initialize();

  int piece = 0;
  int npieces = -1;
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
  {
    npieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }

  if (piece > 0 && npieces >= 1)
  {
    // produce empty dataset.
    return 1;
  }

  if (!this->Internals->Validate())
  {
    return 0;
  }

  vtkSmartPointer<vtkDataObject> data;
  data.TakeReference(vtkDataObjectTypes::NewDataObject(this->PartitionType));
  output->SetBlock(0, data);

  // note: this is an intentional copy of the map since we modify it.
  auto arrays = this->Internals->Arrays;

  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
    arrays.find("coords") != arrays.end() && vtkPointSet::SafeDownCast(data) != nullptr)
  {
    vtkNew<vtkPoints> pts;
    pts->SetData(arrays["coords"]);

    auto ps = vtkPointSet::SafeDownCast(data);
    ps->SetPoints(pts);

    arrays.erase("coords");
  }

  if (auto fd = data->GetAttributesAsFieldData(this->FieldAssociation))
  {
    for (const auto& pair : arrays)
    {
      fd->AddArray(pair.second);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::SetTuple1Double(const char* arrayname, vtkIdType index, double val)
{
  auto& internals = (*this->Internals);
  internals.SetTuple<vtkDoubleArray, 1>(arrayname, index, { val });
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::SetTuple1Int(const char* arrayname, vtkIdType index, int val)
{
  auto& internals = (*this->Internals);
  internals.SetTuple<vtkIntArray, 1>(arrayname, index, { val });
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::SetTuple1IdType(
  const char* arrayname, vtkIdType index, vtkIdType val)
{
  auto& internals = (*this->Internals);
  internals.SetTuple<vtkIdTypeArray, 1>(arrayname, index, { val });
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::SetTuple2Double(
  const char* arrayname, vtkIdType index, double val0, double val1)
{
  auto& internals = (*this->Internals);
  internals.SetTuple<vtkDoubleArray, 2>(arrayname, index, { val0, val1 });
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::SetTuple2Int(
  const char* arrayname, vtkIdType index, int val0, int val1)
{
  auto& internals = (*this->Internals);
  internals.SetTuple<vtkIntArray, 2>(arrayname, index, { val0, val1 });
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::SetTuple2IdType(
  const char* arrayname, vtkIdType index, vtkIdType val0, vtkIdType val1)
{
  auto& internals = (*this->Internals);
  internals.SetTuple<vtkIdTypeArray, 2>(arrayname, index, { val0, val1 });
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::SetTuple3Double(
  const char* arrayname, vtkIdType index, double val0, double val1, double val2)
{
  auto& internals = (*this->Internals);
  internals.SetTuple<vtkDoubleArray, 3>(arrayname, index, { val0, val1, val2 });
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::SetTuple3Int(
  const char* arrayname, vtkIdType index, int val0, int val1, int val2)
{
  auto& internals = (*this->Internals);
  internals.SetTuple<vtkIntArray, 3>(arrayname, index, { val0, val1, val2 });
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::SetTuple3IdType(
  const char* arrayname, vtkIdType index, vtkIdType val0, vtkIdType val1, vtkIdType val2)
{
  auto& internals = (*this->Internals);
  internals.SetTuple<vtkIdTypeArray, 3>(arrayname, index, { val0, val1, val2 });
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::Clear(const char* arrayname)
{
  auto& internals = (*this->Internals);
  internals.Arrays.erase(arrayname);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSteeringDataGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
