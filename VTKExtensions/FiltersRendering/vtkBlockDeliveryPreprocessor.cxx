/*=========================================================================

  Program:   ParaView
  Module:    vtkBlockDeliveryPreprocessor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBlockDeliveryPreprocessor.h"

#include "vtkAttributeDataToTableFilter.h"
#include "vtkExtractBlock.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkUniformGridAMRDataIterator.h"

#include <set>

class vtkBlockDeliveryPreprocessor::CompositeDataSetIndicesType : public std::set<unsigned int>
{
};

vtkStandardNewMacro(vtkBlockDeliveryPreprocessor);
//----------------------------------------------------------------------------
vtkBlockDeliveryPreprocessor::vtkBlockDeliveryPreprocessor()
  : FieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS)
  , FlattenTable(0)
  , GenerateOriginalIds(true)
  , GenerateCellConnectivity(false)
  , SplitComponentsNamingMode(vtkSplitColumnComponents::NAMES_WITH_UNDERSCORES)
  , CompositeDataSetIndices(new vtkBlockDeliveryPreprocessor::CompositeDataSetIndicesType())
{
}

//----------------------------------------------------------------------------
vtkBlockDeliveryPreprocessor::~vtkBlockDeliveryPreprocessor()
{
  delete this->CompositeDataSetIndices;
  this->CompositeDataSetIndices = nullptr;
}

//----------------------------------------------------------------------------
void vtkBlockDeliveryPreprocessor::AddCompositeDataSetIndex(unsigned int index)
{
  if (this->CompositeDataSetIndices->find(index) == this->CompositeDataSetIndices->end())
  {
    this->CompositeDataSetIndices->insert(index);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkBlockDeliveryPreprocessor::RemoveAllCompositeDataSetIndices()
{
  if (this->CompositeDataSetIndices->size() > 0)
  {
    this->CompositeDataSetIndices->clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkBlockDeliveryPreprocessor::RequestDataObject(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_NONE &&
    this->CompositeDataSetIndices->find(0) != this->CompositeDataSetIndices->end() &&
    vtkPartitionedDataSetCollection::GetData(inInfo) != nullptr)

  {
    // this is a hack to pass root node's field data correctly for
    // partitioned-dataset-collections.
    if (vtkTable::GetData(outInfo) == nullptr)
    {
      vtkNew<vtkTable> output;
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    }
    return 1;
  }

  if (vtkPartitionedDataSetCollection::GetData(inInfo))
  {
    if (vtkPartitionedDataSetCollection::GetData(outInfo))
    {
      return 1;
    }

    vtkNew<vtkPartitionedDataSetCollection> output;
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    return 1;
  }
  else if (vtkPartitionedDataSet::GetData(inInfo))
  {
    if (vtkPartitionedDataSet::GetData(outInfo))
    {
      return 1;
    }
    vtkNew<vtkPartitionedDataSet> output;
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    return 1;
  }
  else if (vtkCompositeDataSet::GetData(inInfo))
  {
    if (vtkMultiBlockDataSet::GetData(outInfo))
    {
      return 1;
    }
    vtkNew<vtkMultiBlockDataSet> output;
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    return 1;
  }
  else if (vtkTable::GetData(outInfo))
  {
    return 1;
  }
  else
  {
    vtkNew<vtkTable> output;
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    return 1;
  }
}

//----------------------------------------------------------------------------
int vtkBlockDeliveryPreprocessor::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  // For some reasons, upstream pipeline can produce vtkDataObject
  // instead of a derived type (such case use to appear with vtkMolecule)
  // In this case, data is 'empty' and there is nothing to process.
  if (!strcmp(inputDO->GetClassName(), "vtkDataObject"))
  {
    return 1;
  }

  vtkSmartPointer<vtkDataObject> data = inputDO;
  if (vtkCompositeDataSet::SafeDownCast(data))
  {
    const auto& indices = (*this->CompositeDataSetIndices);
    if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_NONE &&
      indices.find(0) != indices.end() &&
      vtkPartitionedDataSetCollection::SafeDownCast(data) != nullptr)
    {
      // this is a hack. a cleaner solution requires a more aggressive change to
      // this filter. for now, we're just doing this special case for Ioss
      // datasets.
      // not sure what's the best way to deal with field data on the root-node
      // itself.
      vtkNew<vtkTable> table;
      table->GetFieldData()->PassData(data->GetFieldData());
      data = table;
    }
    else if (indices.size() != 0 && indices.find(0) == indices.end())
    {
      // need to extract chosen blocks.
      vtkNew<vtkExtractBlock> eb;
      eb->SetInputData(data);
      for (const auto& index : indices)
      {
        eb->AddIndex(index);
      }
      eb->PruneOutputOff();
      eb->Update();
      data = eb->GetOutputDataObject(0);
    }
  }

  vtkNew<vtkAttributeDataToTableFilter> adtf;
  adtf->SetInputData(data);
  adtf->SetAddMetaData(true);
  adtf->SetGenerateCellConnectivity(this->GenerateCellConnectivity);
  adtf->SetFieldAssociation(this->FieldAssociation);
  adtf->SetGenerateOriginalIds(this->GenerateOriginalIds);
  adtf->Update();
  data = adtf->GetOutputDataObject(0);

  if (this->FlattenTable)
  {
    vtkNew<vtkSplitColumnComponents> split;
    split->SetInputConnection(adtf->GetOutputPort());
    split->SetNamingMode(this->SplitComponentsNamingMode);
    split->Update();
    data = split->GetOutputDataObject(0);
  }

  outputDO->ShallowCopy(data);

  if (auto outputCD = vtkCompositeDataSet::SafeDownCast(outputDO))
  {
    // For composite datasets, we need to add some more meta-data.
    // Add meta-data about composite-index/hierarchical index to help
    // vtkSelectionStreamer.
    vtkCompositeDataIterator* iter = outputCD->NewIterator();
    vtkUniformGridAMRDataIterator* ugIter = vtkUniformGridAMRDataIterator::SafeDownCast(iter);
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkInformation* metaData = iter->GetCurrentMetaData();
      if (metaData)
      {
        metaData->Set(vtkSelectionNode::COMPOSITE_INDEX(), iter->GetCurrentFlatIndex());
        if (ugIter)
        {
          metaData->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(), ugIter->GetCurrentLevel());
          metaData->Set(vtkSelectionNode::HIERARCHICAL_INDEX(), ugIter->GetCurrentIndex());
        }
      }
    }
    iter->Delete();
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkBlockDeliveryPreprocessor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
