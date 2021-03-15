/*=========================================================================

  Program:   ParaView
  Module:    vtkDataTabulator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataTabulator.h"

#include "vtkAttributeDataToTableFilter.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkExtractBlockUsingDataAssembly.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSplitColumnComponents.h"
#include "vtkTable.h"
#include "vtkUniformGridAMR.h"
#include "vtkUniformGridAMRDataIterator.h"

#include <cassert>
#include <map>
#include <string>

namespace
{
vtkTable* NewFieldDataWrapper(vtkDataObject* dobj)
{
  vtkTable* dummy = vtkTable::New();
  dummy->GetFieldData()->PassData(dobj->GetFieldData());
  return dummy;
}

std::map<vtkDataObject*, std::string> GenerateNameMap(vtkDataObject* dobj)
{
  std::map<vtkDataObject*, std::string> result;
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(dobj);
  if (!pdc)
  {
    return result;
  }

  for (unsigned int cc = 0; cc < pdc->GetNumberOfPartitionedDataSets(); ++cc)
  {
    if (pdc->GetPartitionedDataSet(cc) && pdc->GetMetaData(cc) &&
      pdc->GetMetaData(cc)->Has(vtkCompositeDataSet::NAME()))
    {
      auto ptd = pdc->GetPartitionedDataSet(cc);
      std::string name = pdc->GetMetaData(cc)->Get(vtkCompositeDataSet::NAME());
      for (unsigned int idx = 0; idx < ptd->GetNumberOfPartitions(); ++idx)
      {
        if (auto part = ptd->GetPartitionAsDataObject(idx))
        {
          result[part] = name;
        }
      }
    }
  }
  return result;
}
}

vtkStandardNewMacro(vtkDataTabulator);
vtkInformationKeyMacro(vtkDataTabulator, COMPOSITE_INDEX, Integer);
vtkInformationKeyMacro(vtkDataTabulator, HIERARCHICAL_LEVEL, Integer);
vtkInformationKeyMacro(vtkDataTabulator, HIERARCHICAL_INDEX, Integer);
//----------------------------------------------------------------------------
vtkDataTabulator::vtkDataTabulator()
  : FieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS)
  , GenerateCellConnectivity(false)
  , GenerateOriginalIds(true)
  , SplitComponents(false)
  , SplitComponentsNamingMode(vtkSplitColumnComponents::NAMES_WITH_UNDERSCORES)
  , ActiveAssemblyForSelectors(nullptr)
{
  this->SetActiveAssemblyForSelectors("Hierarchy");
}

//----------------------------------------------------------------------------
vtkDataTabulator::~vtkDataTabulator()
{
  this->SetActiveAssemblyForSelectors(nullptr);
}

//----------------------------------------------------------------------------
int vtkDataTabulator::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataTabulator::AddSelector(const char* selector)
{
  if (selector && this->Selectors.insert(selector).second)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkDataTabulator::ClearSelectors()
{
  if (this->Selectors.size() > 0)
  {
    this->Selectors.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkDataTabulator::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto outputPD = vtkPartitionedDataSet::GetData(outputVector, 0);

  if (vtkSmartPointer<vtkCompositeDataSet> inputCD = vtkCompositeDataSet::SafeDownCast(inputDO))
  {
    if (this->Selectors.size() > 0)
    {
      vtkNew<vtkExtractBlockUsingDataAssembly> extractor;
      extractor->SetAssemblyName(this->ActiveAssemblyForSelectors);
      for (const auto& selector : this->Selectors)
      {
        extractor->AddSelector(selector.c_str());
      }
      extractor->SetInputData(inputCD);
      extractor->Update();
      inputCD = vtkCompositeDataSet::SafeDownCast(extractor->GetOutputDataObject(0));
    }

    // we need special handling for names for partitioned-dataset in a
    // partitioned-dataset-collection. to handle that we build this map.
    auto nameMap = ::GenerateNameMap(inputDO);

    vtkNew<vtkPartitionedDataSet> xInput;

    auto iter = inputCD->NewIterator();
    auto treeIter = vtkDataObjectTreeIterator::SafeDownCast(iter);
    auto amrIter = vtkUniformGridAMRDataIterator::SafeDownCast(iter);

    const bool global_data =
      (treeIter && this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_NONE);
    if (global_data)
    {
      treeIter->VisitOnlyLeavesOff();

      // since root node it skipped when iterating, handle it explicitly.
      if (inputCD->GetFieldData()->GetNumberOfTuples() > 0)
      {
        auto dummy = ::NewFieldDataWrapper(inputCD);
        xInput->SetPartition(0, dummy);
        dummy->FastDelete();

        auto metaData = xInput->GetMetaData(0u);
        metaData->Set(vtkDataTabulator::COMPOSITE_INDEX(), 0);
        metaData->Set(vtkCompositeDataSet::NAME(), "root");
      }
    }

    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      const auto nextIdx = xInput->GetNumberOfPartitions();
      auto currentDO = iter->GetCurrentDataObject();
      auto currentMD = iter->GetCurrentMetaData();
      if (global_data && vtkCompositeDataSet::SafeDownCast(currentDO) != nullptr)
      {
        if (currentDO->GetFieldData()->GetNumberOfTuples() == 0)
        {
          continue;
        }

        auto dummy = ::NewFieldDataWrapper(currentDO);
        xInput->SetPartition(nextIdx, dummy);
        dummy->FastDelete();
      }
      else
      {
        xInput->SetPartition(nextIdx, currentDO);
      }

      auto metaData = xInput->GetMetaData(nextIdx);
      if (currentMD)
      {
        // copy existing metadata first e.g. name etc.
        metaData->Copy(currentMD);
      }

      // now add keys to help identify original composite index.
      metaData->Set(vtkDataTabulator::COMPOSITE_INDEX(), iter->GetCurrentFlatIndex());
      if (amrIter)
      {
        metaData->Set(vtkDataTabulator::HIERARCHICAL_LEVEL(), amrIter->GetCurrentLevel());
        metaData->Set(vtkDataTabulator::HIERARCHICAL_INDEX(), amrIter->GetCurrentIndex());
      }

      if (!metaData->Has(vtkCompositeDataSet::NAME()))
      {
        auto niter = nameMap.find(currentDO);
        if (niter == nameMap.end())
        {
          // if a name doesn't exist, let's create one.
          std::string name = amrIter
            ? "level=" + std::to_string(amrIter->GetCurrentLevel()) + " index=" +
              std::to_string(amrIter->GetCurrentIndex())
            : "unnamed_" + std::to_string(iter->GetCurrentFlatIndex());
          metaData->Set(vtkCompositeDataSet::NAME(), name.c_str());
        }
        else
        {
          metaData->Set(vtkCompositeDataSet::NAME(), niter->second.c_str());
        }
      }
    }
    iter->Delete();

    auto output = this->Transform(xInput);
    outputPD->ShallowCopy(output);
  }
  else
  {
    auto block = this->Transform(inputDO);
    assert(vtkCompositeDataSet::SafeDownCast(block) == nullptr);
    outputPD->SetPartition(0, block);
  }

  return 1;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataObject> vtkDataTabulator::Transform(vtkDataObject* data)
{
  vtkNew<vtkAttributeDataToTableFilter> adtf;
  adtf->SetInputData(data);
  adtf->SetAddMetaData(true);
  adtf->SetGenerateCellConnectivity(this->GenerateCellConnectivity);
  adtf->SetFieldAssociation(this->FieldAssociation);
  adtf->SetGenerateOriginalIds(this->GenerateOriginalIds);

  if (this->SplitComponents)
  {
    vtkNew<vtkSplitColumnComponents> splitter;
    splitter->SetInputConnection(adtf->GetOutputPort());
    splitter->SetNamingMode(this->SplitComponentsNamingMode);
    splitter->Update();
    return splitter->GetOutputDataObject(0);
  }

  adtf->Update();
  return adtf->GetOutputDataObject(0);
}

//----------------------------------------------------------------------------
bool vtkDataTabulator::HasInputCompositeIds(vtkPartitionedDataSet* ptd)
{
  return (ptd && ptd->GetNumberOfPartitions() > 0 && ptd->HasMetaData(0u) &&
    ptd->GetMetaData(0u)->Has(vtkDataTabulator::COMPOSITE_INDEX()));
}

//----------------------------------------------------------------------------
void vtkDataTabulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
