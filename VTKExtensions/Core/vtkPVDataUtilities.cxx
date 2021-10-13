/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataUtilities.h"

#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"

#include <cmath>

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/core.h)
// clang-format on

namespace
{
template <typename IntegralType>
int GetNumberOfDigits(IntegralType value)
{
  return value > 0 ? 1 + static_cast<IntegralType>(std::log(value)) : 1;
}

void AssignNamesToBlocks(vtkUniformGridAMR* amr)
{
  const auto numLevels = amr->GetNumberOfLevels();
  for (unsigned int l = 0; l < numLevels; ++l)
  {
    const std::string levelString = fmt::format("{:{}}", l, GetNumberOfDigits(l));
    const auto numDataSets = amr->GetNumberOfDataSets(l);
    for (unsigned idx = 0; idx < numDataSets; ++idx)
    {
      const std::string label =
        fmt::format("amr_index {} {:{}}", levelString, idx, GetNumberOfDigits(idx));
      if (auto ds = amr->GetDataSet(l, idx))
      {
        ds->GetInformation()->Set(vtkCompositeDataSet::NAME(), label.c_str());
      }
    }
  }
}

void AssignNamesToBlocks(vtkMultiPieceDataSet* mp, const std::string& parentName = {})
{
  const auto numPieces = mp->GetNumberOfPieces();
  for (unsigned int piece = 0; piece < numPieces; ++piece)
  {
    if (auto ds = mp->GetPieceAsDataObject(piece))
    {
      if (mp->HasMetaData(piece) && mp->GetMetaData(piece)->Has(vtkCompositeDataSet::NAME()))
      {
        ds->GetInformation()->Set(
          vtkCompositeDataSet::NAME(), mp->GetMetaData(piece)->Get(vtkCompositeDataSet::NAME()));
      }
      else if (!parentName.empty())
      {
        ds->GetInformation()->Set(vtkCompositeDataSet::NAME(), parentName.c_str());
      }
      else
      {
        const std::string pieceString =
          fmt::format("unnamed_piece_{:{}}", piece, GetNumberOfDigits(piece));
        ds->GetInformation()->Set(vtkCompositeDataSet::NAME(), pieceString.c_str());
      }
    }
  }
}

void AssignNamesToBlocks(vtkPartitionedDataSet* pds, const std::string& parentName = {})
{
  const auto numPartitions = pds->GetNumberOfPartitions();
  for (unsigned int idx = 0; idx < numPartitions; ++idx)
  {
    if (auto ds = pds->GetPartitionAsDataObject(idx))
    {
      if (pds->HasMetaData(idx) && pds->GetMetaData(idx)->Has(vtkCompositeDataSet::NAME()))
      {
        ds->GetInformation()->Set(
          vtkCompositeDataSet::NAME(), pds->GetMetaData(idx)->Get(vtkCompositeDataSet::NAME()));
      }
      else if (!parentName.empty())
      {
        ds->GetInformation()->Set(vtkCompositeDataSet::NAME(), parentName.c_str());
      }
      else
      {
        const std::string label =
          fmt::format("unnamed_partition_{:{}}", idx, GetNumberOfDigits(idx));
        ds->GetInformation()->Set(vtkCompositeDataSet::NAME(), label.c_str());
      }
    }
  }
}

void AssignNamesToBlocks(vtkPartitionedDataSetCollection* pdc)
{
  const auto numPDS = pdc->GetNumberOfPartitionedDataSets();
  for (unsigned int idx = 0; idx < numPDS; ++idx)
  {
    const std::string blockName =
      (pdc->HasMetaData(idx) && pdc->GetMetaData(idx)->Has(vtkCompositeDataSet::NAME()))
      ? std::string(pdc->GetMetaData(idx)->Get(vtkCompositeDataSet::NAME()))
      : fmt::format("unnamed_block_{:{}}", idx, GetNumberOfDigits(idx));
    if (auto pds = pdc->GetPartitionedDataSet(idx))
    {
      ::AssignNamesToBlocks(pds, blockName);
    }
  }
}

void AssignNamesToBlocks(vtkMultiBlockDataSet* mb)
{
  const auto numBlocks = mb->GetNumberOfBlocks();
  for (unsigned int idx = 0; idx < numBlocks; ++idx)
  {
    const std::string blockName =
      (mb->HasMetaData(idx) && mb->GetMetaData(idx)->Has(vtkCompositeDataSet::NAME()))
      ? std::string(mb->GetMetaData(idx)->Get(vtkCompositeDataSet::NAME()))
      : fmt::format("unnamed_block_{:{}}", idx, GetNumberOfDigits(idx));

    auto block = mb->GetBlock(idx);
    if (auto childMB = vtkMultiBlockDataSet::SafeDownCast(block))
    {
      ::AssignNamesToBlocks(childMB);
    }
    else if (auto mp = vtkMultiPieceDataSet::SafeDownCast(block))
    {
      ::AssignNamesToBlocks(mp, blockName);
    }
    else if (block)
    {
      block->GetInformation()->Set(vtkCompositeDataSet::NAME(), blockName.c_str());
    }
  }
}
}

vtkStandardNewMacro(vtkPVDataUtilities);
//----------------------------------------------------------------------------
vtkPVDataUtilities::vtkPVDataUtilities() = default;

//----------------------------------------------------------------------------
vtkPVDataUtilities::~vtkPVDataUtilities() = default;

//----------------------------------------------------------------------------
void vtkPVDataUtilities::AssignNamesToBlocks(vtkDataObject* dataObject)
{
  auto cd = vtkCompositeDataSet::SafeDownCast(dataObject);
  if (!cd)
  {
    return;
  }

  if (auto amr = vtkUniformGridAMR::SafeDownCast(cd))
  {
    ::AssignNamesToBlocks(amr);
  }
  else if (auto mp = vtkMultiPieceDataSet::SafeDownCast(cd))
  {
    ::AssignNamesToBlocks(mp);
  }
  else if (auto pds = vtkPartitionedDataSet::SafeDownCast(cd))
  {
    ::AssignNamesToBlocks(pds);
  }
  else if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(cd))
  {
    ::AssignNamesToBlocks(pdc);
  }
  else if (auto mb = vtkMultiBlockDataSet::SafeDownCast(cd))
  {
    ::AssignNamesToBlocks(mb);
  }
}

//----------------------------------------------------------------------------
std::string vtkPVDataUtilities::GetAssignedNameForBlock(vtkDataObject* block)
{
  if (block && block->GetInformation()->Has(vtkCompositeDataSet::NAME()))
  {
    return block->GetInformation()->Get(vtkCompositeDataSet::NAME());
  }

  return {};
}

//----------------------------------------------------------------------------
void vtkPVDataUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
