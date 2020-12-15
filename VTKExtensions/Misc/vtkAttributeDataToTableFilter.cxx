/*=========================================================================

  Program:   ParaView
  Module:    vtkAttributeDataToTableFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAttributeDataToTableFilter.h"

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkAttributeDataToTableFilter);
//----------------------------------------------------------------------------
vtkAttributeDataToTableFilter::vtkAttributeDataToTableFilter()
{
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  this->AddMetaData = false;
  this->GenerateOriginalIds = false;
  this->GenerateCellConnectivity = false;
}

//----------------------------------------------------------------------------
vtkAttributeDataToTableFilter::~vtkAttributeDataToTableFilter()
{
}

//----------------------------------------------------------------------------
vtkExecutive* vtkAttributeDataToTableFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int vtkAttributeDataToTableFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

//----------------------------------------------------------------------------
int vtkAttributeDataToTableFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkTable* output = vtkTable::GetData(outputVector);
  if (vtkFieldData* fieldData = input->GetAttributesAsFieldData(this->FieldAssociation))
  {
    if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_NONE)
    {
      // Field data can have different length arrays, so we need to create
      // output vtkTable big enough to fit the largest array.
      this->PassFieldData(output->GetRowData(), fieldData);
    }
    else
    {
      output->GetRowData()->ShallowCopy(fieldData);
      vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
      if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS && ds != nullptr)
      {
        this->AddCellTypeAndConnectivity(output, ds);
      }
    }

    // Clear any attribute markings from the output. This resolves the problem
    // that GlobalNodeIds were not showing up in spreadsheet view.
    for (int cc = vtkDataSetAttributes::SCALARS; cc < vtkDataSetAttributes::NUM_ATTRIBUTES; cc++)
    {
      output->GetRowData()->SetActiveAttribute(-1, cc);
    }

    if (this->AddMetaData && this->FieldAssociation != vtkDataObject::FIELD_ASSOCIATION_NONE)
    {
      this->Decorate(output, input);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkAttributeDataToTableFilter::AddCellTypeAndConnectivity(vtkTable* output, vtkDataSet* ds)
{
  vtkNew<vtkCharArray> celltypes;
  celltypes->SetName("Cell Type");
  const vtkIdType numcells = ds->GetNumberOfCells();
  celltypes->SetNumberOfTuples(numcells);
  char* ptr = celltypes->GetPointer(0);
  vtkNew<vtkIdList> points;
  vtkIdType maxpoints = 0;
  for (vtkIdType cc = 0; cc < numcells; cc++)
  {
    ptr[cc] = static_cast<char>(ds->GetCellType(cc));
    ds->GetCellPoints(cc, points);
    maxpoints = maxpoints > points->GetNumberOfIds() ? maxpoints : points->GetNumberOfIds();
  }
  output->GetRowData()->AddArray(celltypes);

  if (this->GenerateCellConnectivity)
  {
    std::vector<vtkSmartPointer<vtkIdTypeArray> > indices(maxpoints);
    int w = 1 + log10(maxpoints);
    for (vtkIdType i = 0; i < maxpoints; i++)
    {
      std::stringstream arrayname;
      arrayname << "Point Index " << std::setw(w) << std::setfill('0') << i;
      vtkNew<vtkIdTypeArray> idarray;
      idarray->SetName(arrayname.str().c_str());
      idarray->SetNumberOfTuples(numcells);
      indices[i] = idarray;
    }

    for (vtkIdType cc = 0; cc < numcells; cc++)
    {
      ds->GetCellPoints(cc, points);
      for (vtkIdType pt = 0; pt < maxpoints; pt++)
      {
        if (pt < points->GetNumberOfIds())
        {
          indices[pt]->SetValue(cc, points->GetId(pt));
        }
        else
        {
          indices[pt]->SetValue(cc, -1);
        }
      }
    }
    for (int i = 0; i < maxpoints; i++)
    {
      this->ConvertToOriginalIds(ds, indices[i]);
      output->GetRowData()->AddArray(indices[i]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkAttributeDataToTableFilter::PassFieldData(vtkFieldData* output, vtkFieldData* input)
{
  output->DeepCopy(input);

  // Now resize arrays to match the longest one.
  vtkIdType max_count = 0;
  for (int cc = 0, max = output->GetNumberOfArrays(); cc < max; ++cc)
  {
    if (vtkAbstractArray* arr = output->GetAbstractArray(cc))
    {
      max_count = std::max(max_count, arr->GetNumberOfTuples());
    }
  }

  for (int cc = 0, max = output->GetNumberOfArrays(); cc < max; ++cc)
  {
    vtkAbstractArray* arr = output->GetAbstractArray(cc);
    if (!arr)
    {
      continue;
    }

    const vtkIdType current_count = arr->GetNumberOfTuples();
    if ((current_count != max_count) && arr->Resize(max_count))
    {
      arr->SetNumberOfTuples(max_count);

      vtkNew<vtkUnsignedCharArray> maskArray;
      maskArray->SetName(("__vtkValidMask__" + std::string(arr->GetName())).c_str());
      maskArray->SetNumberOfTuples(max_count);
      maskArray->FillValue(static_cast<unsigned char>(1));
      output->AddArray(maskArray);

      const int num_comps = arr->GetNumberOfComponents();
      vtkDataArray* da = vtkDataArray::SafeDownCast(arr);
      if (da != nullptr && num_comps > 0)
      {
        std::vector<double> tuple(num_comps, 0.0);
        for (vtkIdType jj = current_count; jj < max_count; ++jj)
        {
          da->SetTuple(jj, &tuple[0]);
          maskArray->SetTypedComponent(jj, 0, static_cast<unsigned char>(0));
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkAttributeDataToTableFilter::Decorate(vtkTable* output, vtkDataObject* input)
{
  vtkPointSet* psInput = vtkPointSet::SafeDownCast(input);
  vtkRectilinearGrid* rgInput = vtkRectilinearGrid::SafeDownCast(input);
  vtkImageData* idInput = vtkImageData::SafeDownCast(input);
  vtkStructuredGrid* sgInput = vtkStructuredGrid::SafeDownCast(input);
  const int* dimensions = 0;
  if (rgInput)
  {
    dimensions = rgInput->GetDimensions();
  }
  else if (idInput)
  {
    dimensions = idInput->GetDimensions();
  }
  else if (sgInput)
  {
    dimensions = sgInput->GetDimensions();
  }

  int cellDims[3];
  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS && dimensions)
  {
    cellDims[0] = std::max(1, (dimensions[0] - 1));
    cellDims[1] = std::max(1, (dimensions[1] - 1));
    cellDims[2] = std::max(1, (dimensions[2] - 1));
    dimensions = cellDims;
  }

  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS && psInput &&
    psInput->GetPoints())
  {
    output->GetRowData()->AddArray(psInput->GetPoints()->GetData());
  }

  if (dimensions)
  {
    // I cannot decide if this should be put in the vtkInformation associated
    // with the vtkTable or in FieldData. I'd rather the former but not sure
    // how that's going to be propagated through the pipeline.
    vtkIntArray* dArray = vtkIntArray::New();
    dArray->SetName("STRUCTURED_DIMENSIONS");
    dArray->SetNumberOfComponents(3);
    dArray->SetNumberOfTuples(1);
    dArray->SetTypedTuple(0, dimensions);
    output->GetFieldData()->AddArray(dArray);
    dArray->Delete();
  }

  if (this->GenerateOriginalIds)
  {
    // Add an original ids array. I know, this is going to bloat up the memory a
    // bit, but it's much easier to add the array now than later (esp. in
    // vtkSortedTableStreamer) since it's ending up buggy and hard to track.

    vtkIdTypeArray* indicesArray = vtkIdTypeArray::New();
    indicesArray->SetName("vtkOriginalIndices");
    indicesArray->SetNumberOfComponents(1);
    vtkIdType numElements = input->GetNumberOfElements(this->FieldAssociation);
    indicesArray->SetNumberOfTuples(numElements);
    for (vtkIdType cc = 0; cc < numElements; cc++)
    {
      indicesArray->SetValue(cc, cc);
    }
    output->GetRowData()->AddArray(indicesArray);
    indicesArray->FastDelete();
  }
}

//----------------------------------------------------------------------------
void vtkAttributeDataToTableFilter::ConvertToOriginalIds(
  vtkDataSet* inputDS, vtkIdTypeArray* indices)
{
  vtkPointData* pd = inputDS->GetPointData();
  if (!pd)
  {
    return;
  }

  vtkDataArray* originalIds = pd->GetArray("vtkOriginalPointIds");
  if (!originalIds)
  {
    originalIds = pd->GetArray("vtkOriginalIndices");
  }
  if (originalIds)
  {
    for (vtkIdType i = 0; i < indices->GetNumberOfValues(); ++i)
    {
      vtkIdType id = indices->GetValue(i);
      if (id >= 0 && id < originalIds->GetNumberOfTuples())
      {
        vtkIdType origId = originalIds->GetComponent(id, 0);
        indices->SetValue(i, origId);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkAttributeDataToTableFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldAssociation: " << this->FieldAssociation << endl;
  os << indent << "AddMetaData: " << this->AddMetaData << endl;
  os << indent << "GenerateOriginalIds: " << this->GenerateOriginalIds << endl;
}
