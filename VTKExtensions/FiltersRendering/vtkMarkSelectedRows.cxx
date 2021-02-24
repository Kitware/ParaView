/*=========================================================================

  Program:   ParaView
  Module:    vtkMarkSelectedRows.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMarkSelectedRows.h"

#include "vtkCharArray.h"
#include "vtkDataTabulator.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkTable.h"

#include <cassert>
#include <map>

namespace
{

const char* GetOriginalIdsArrayName(int association)
{
  switch (association)
  {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      return "vtkOriginalPointIds";

    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      return "vtkOriginalCellIds";

    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      return "vtkOriginalRowIds";

    default:
      return nullptr;
  }
}

inline unsigned int GetCompositeId(vtkPartitionedDataSet* ptd, unsigned int idx)
{
  if (ptd->HasMetaData(idx))
  {
    auto info = ptd->GetMetaData(idx);
    return info->Has(vtkDataTabulator::COMPOSITE_INDEX())
      ? info->Get(vtkDataTabulator::COMPOSITE_INDEX())
      : 0;
  }
  return 0;
}

std::map<unsigned int, vtkTable*> BuildMap(vtkPartitionedDataSet* ptd)
{
  std::map<unsigned int, vtkTable*> data_map;
  for (unsigned int cc = 0, max = ptd->GetNumberOfPartitions(); cc < max; ++cc)
  {
    const unsigned int cid = GetCompositeId(ptd, cc);
    assert(data_map.find(cid) == data_map.end());
    data_map[cid] = vtkTable::SafeDownCast(ptd->GetPartitionAsDataObject(cc));
  }
  return data_map;
}
}

vtkStandardNewMacro(vtkMarkSelectedRows);
//----------------------------------------------------------------------------
vtkMarkSelectedRows::vtkMarkSelectedRows()
  : FieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS)
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkMarkSelectedRows::~vtkMarkSelectedRows() = default;

//----------------------------------------------------------------------------
int vtkMarkSelectedRows::FillInputPortInformation(int port, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkMarkSelectedRows::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkMarkSelectedRows::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkPartitionedDataSet::GetData(inputVector[0], 0);
  auto extractedInput = vtkPartitionedDataSet::GetData(inputVector[1], 0);
  auto output = vtkPartitionedDataSet::GetData(outputVector, 0);

  const char* originalIdArrayName = ::GetOriginalIdsArrayName(this->FieldAssociation);

  if (extractedInput == nullptr || originalIdArrayName == nullptr ||
    extractedInput->GetNumberOfElements(vtkDataObject::ROW) == 0)
  {
    // the extracted input doesn't exist, no need to mark anything selected.
    output->ShallowCopy(input);
    return 1;
  }

  auto extractedInputMap = ::BuildMap(extractedInput);
  output->CopyStructure(input);
  for (unsigned int cc = 0, max = input->GetNumberOfPartitions(); cc < max; ++cc)
  {
    auto inputTable = vtkTable::SafeDownCast(input->GetPartitionAsDataObject(cc));
    if (!inputTable)
    {
      continue;
    }

    const auto numRows = inputTable->GetNumberOfRows();
    auto clone = vtkTable::New();
    clone->ShallowCopy(inputTable);

    vtkCharArray* selected = vtkCharArray::New();
    selected->SetName("__vtkIsSelected__");
    selected->SetNumberOfTuples(numRows);
    selected->FillValue(0);

    clone->AddColumn(selected);
    selected->FastDelete();

    output->SetPartition(cc, clone);
    clone->FastDelete();

    if (numRows == 0)
    {
      continue;
    }

    auto cid = ::GetCompositeId(input, cc);
    if (extractedInputMap.find(cid) == extractedInputMap.end())
    {
      continue;
    }

    auto extractedTable = extractedInputMap[cid];
    auto selectedIds =
      vtkIdTypeArray::SafeDownCast(extractedTable->GetColumnByName(originalIdArrayName));
    if (!selectedIds || selectedIds->GetNumberOfTuples() == 0)
    {
      continue;
    }

    auto inputIds = vtkIdTypeArray::SafeDownCast(clone->GetColumnByName("vtkOriginalIndices"));
    for (vtkIdType idx = 0; idx < numRows; ++idx)
    {
      vtkIdType id = inputIds ? inputIds->GetTypedComponent(idx, 0) : idx;
      if (selectedIds->LookupTypedValue(id) != -1)
      {
        selected->SetTypedComponent(idx, 0, 1);
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkMarkSelectedRows::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldAssociation: " << this->FieldAssociation << endl;
}
