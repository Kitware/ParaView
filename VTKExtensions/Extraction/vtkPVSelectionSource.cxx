/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSelectionSource.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#include <map>
#include <set>
#include <vector>

class vtkPVSelectionSource::vtkInternal
{
public:
  struct PieceIdType
  {
    vtkIdType Piece;
    vtkIdType ID;
    PieceIdType(vtkIdType piece, vtkIdType id)
      : Piece(piece)
      , ID(id)
    {
    }

    bool operator<(const PieceIdType& other) const
    {
      if (this->Piece == other.Piece)
      {
        return (this->ID < other.ID);
      }
      return (this->Piece < other.Piece);
    }
  };

  struct HierarchicalIDType
  {
    unsigned int Level;
    unsigned int DataSet;
    HierarchicalIDType(unsigned int level, unsigned int ds)
      : Level(level)
      , DataSet(ds)
    {
    }
    bool operator<(const HierarchicalIDType& other) const
    {
      if (this->Level == other.Level)
      {
        return (this->DataSet < other.DataSet);
      }
      return (this->Level < other.Level);
    }
  };

  using IdType = vtkIdType;
  using StringIDType = std::string;
  using DomainType = std::string;
  using CompositeIDType = unsigned int;
  using SetOfIDs = std::set<vtkIdType>;
  using SetOfPieceIDs = std::set<PieceIdType>;
  using MapOfCompositeIDs = std::map<CompositeIDType, std::set<PieceIdType>>;
  using MapOfHierarchicalIDs = std::map<HierarchicalIDType, std::set<IdType>>;
  using SetOfPedigreeIDs = std::map<DomainType, std::set<IdType>>;
  using SetOfPedigreeStringIDs = std::map<DomainType, std::set<StringIDType>>;
  using VectorOfDoubles = std::vector<double>;
  using VectorOfStrings = std::vector<std::string>;

  SetOfIDs GlobalIDs;
  SetOfIDs Blocks;
  SetOfPieceIDs IDs;
  SetOfPieceIDs Values;
  MapOfCompositeIDs CompositeIDs;
  MapOfHierarchicalIDs HierarchicalIDs;
  SetOfPedigreeIDs PedigreeIDs;
  SetOfPedigreeStringIDs PedigreeStringIDs;
  VectorOfDoubles Locations;
  VectorOfDoubles Thresholds;
  VectorOfStrings BlockSelectors;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSelectionSource);

//----------------------------------------------------------------------------
vtkPVSelectionSource::vtkPVSelectionSource()
  : Mode(Modes::ID)
  , FieldType(vtkSelectionNode::CELL)
  , ContainingCells(0)
  , Inverse(0)
  , ArrayName(nullptr)
  , NumberOfLayers(0)
  , ProcessID(-1)
  , Internal(new vtkInternal())
{
  memset(this->Frustum, 0.0, sizeof(double) * 32);
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkPVSelectionSource::~vtkPVSelectionSource()
{
  this->SetArrayName(nullptr);
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddFrustum(double vertices[32])
{
  memcpy(this->Frustum, vertices, sizeof(double) * 32);
  this->Mode = FRUSTUM;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddGlobalID(vtkIdType id)
{
  this->Mode = GLOBALIDS;
  this->Internal->GlobalIDs.insert(id);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllGlobalIDs()
{
  this->Mode = GLOBALIDS;
  this->Internal->GlobalIDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddPedigreeID(const char* domain, vtkIdType id)
{
  this->Mode = PEDIGREEIDS;
  this->Internal->PedigreeIDs[domain].insert(id);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllPedigreeIDs()
{
  this->Mode = PEDIGREEIDS;
  this->Internal->PedigreeIDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddPedigreeStringID(const char* domain, const char* id)
{
  this->Mode = PEDIGREEIDS;
  this->Internal->PedigreeStringIDs[domain].insert(id);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllPedigreeStringIDs()
{
  this->Mode = PEDIGREEIDS;
  this->Internal->PedigreeStringIDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddID(vtkIdType piece, vtkIdType id)
{
  if (piece < -1)
  {
    piece = -1;
  }
  this->Mode = ID;
  this->Internal->IDs.insert(vtkInternal::PieceIdType(piece, id));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllIDs()
{
  this->Mode = ID;
  this->Internal->IDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddValue(vtkIdType piece, vtkIdType value)
{
  if (piece < -1)
  {
    piece = -1;
  }
  this->Mode = VALUES;
  this->Internal->Values.insert(vtkInternal::PieceIdType(piece, value));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllValues()
{
  this->Mode = VALUES;
  this->Internal->Values.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddCompositeID(
  unsigned int composite_index, vtkIdType piece, vtkIdType id)
{
  if (piece < -1)
  {
    piece = -1;
  }

  this->Mode = COMPOSITEID;
  this->Internal->CompositeIDs[composite_index].insert(vtkInternal::PieceIdType(piece, id));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllCompositeIDs()
{
  this->Mode = COMPOSITEID;
  this->Internal->CompositeIDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddHierarhicalID(unsigned int level, unsigned int dataset, vtkIdType id)
{
  this->Mode = HIERARCHICALID;
  this->Internal->HierarchicalIDs[vtkInternal::HierarchicalIDType(level, dataset)].insert(id);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllHierarchicalIDs()
{
  this->Mode = HIERARCHICALID;
  this->Internal->HierarchicalIDs.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddBlock(vtkIdType blockno)
{
  this->Mode = BLOCKS;
  this->Internal->Blocks.insert(blockno);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllBlocks()
{
  this->Mode = BLOCKS;
  this->Internal->Blocks.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddThreshold(double min, double max)
{
  this->Mode = THRESHOLDS;
  this->Internal->Thresholds.push_back(min);
  this->Internal->Thresholds.push_back(max);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllThresholds()
{
  this->Mode = THRESHOLDS;
  this->Internal->Thresholds.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddLocation(double x, double y, double z)
{
  this->Mode = LOCATIONS;
  this->Internal->Locations.push_back(x);
  this->Internal->Locations.push_back(y);
  this->Internal->Locations.push_back(z);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllLocations()
{
  this->Mode = LOCATIONS;
  this->Internal->Locations.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::AddBlockSelector(const char* selector)
{
  if (selector)
  {
    this->Mode = BLOCK_SELECTORS;
    this->Internal->BlockSelectors.push_back(selector);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::RemoveAllBlockSelectors()
{
  this->Internal->BlockSelectors.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPVSelectionSource::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVSelectionSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkSelection* output = vtkSelection::GetData(outputVector);
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

  vtkNew<vtkSelectionSource> source;
  source->SetFieldType(this->FieldType);
  source->SetProcessID(this->ProcessID);
  source->SetContainingCells(this->ContainingCells);
  source->SetInverse(this->Inverse);
  source->SetNumberOfLayers(this->NumberOfLayers);
  source->SetRemoveSeed(this->RemoveSeed);
  source->SetRemoveIntermediateLayers(this->RemoveIntermediateLayers);

  switch (this->Mode)
  {
    case FRUSTUM:
    {
      source->SetContentType(vtkSelectionNode::FRUSTUM);
      source->SetFrustum(this->Frustum);
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case GLOBALIDS:
    {
      source->SetContentType(vtkSelectionNode::GLOBALIDS);
      for (const auto& globalId : this->Internal->GlobalIDs)
      {
        source->AddID(-1, globalId);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case PEDIGREEIDS:
    {
      // Add integer IDs
      if (!this->Internal->PedigreeIDs.empty())
      {
        source->SetNumberOfNodes(static_cast<unsigned int>(this->Internal->PedigreeIDs.size()));
        unsigned int nodeId = 0;
        for (const auto& idsOfSpecificPedigree : this->Internal->PedigreeIDs)
        {
          source->SetContentType(nodeId, vtkSelectionNode::PEDIGREEIDS);
          source->SetContainingCells(nodeId, this->ContainingCells);
          source->SetInverse(nodeId, this->Inverse);
          source->SetNumberOfLayers(nodeId, this->NumberOfLayers);
          source->SetRemoveSeed(this->RemoveSeed);
          source->SetRemoveIntermediateLayers(this->RemoveIntermediateLayers);
          source->SetArrayName(nodeId, idsOfSpecificPedigree.first.c_str());
          for (const auto& id : idsOfSpecificPedigree.second)
          {
            source->AddID(nodeId, -1, id);
          }
          ++nodeId;
        }
        source->UpdatePiece(piece, npieces, 0);
        output->ShallowCopy(source->GetOutput());
      }
      // Add String IDs
      else if (!this->Internal->PedigreeStringIDs.empty())
      {
        source->SetNumberOfNodes(
          static_cast<unsigned int>(this->Internal->PedigreeStringIDs.size()));
        unsigned int nodeId = 0;
        for (const auto& stringIdsOfSpecificPedigree : this->Internal->PedigreeStringIDs)
        {
          source->SetContentType(nodeId, vtkSelectionNode::PEDIGREEIDS);
          source->SetContainingCells(nodeId, this->ContainingCells);
          source->SetInverse(nodeId, this->Inverse);
          source->SetNumberOfLayers(nodeId, this->NumberOfLayers);
          source->SetRemoveSeed(this->RemoveSeed);
          source->SetRemoveIntermediateLayers(this->RemoveIntermediateLayers);
          source->SetArrayName(nodeId, stringIdsOfSpecificPedigree.first.c_str());
          for (const auto& stringId : stringIdsOfSpecificPedigree.second)
          {
            source->AddStringID(nodeId, -1, stringId.c_str());
          }
          ++nodeId;
        }
        source->UpdatePiece(piece, npieces, 0);
        output->ShallowCopy(source->GetOutput());
      }
    }
    break;

    case COMPOSITEID:
    {
      source->SetNumberOfNodes(static_cast<int>(this->Internal->CompositeIDs.size()));
      unsigned int nodeId = 0;
      for (const auto& pieceIdsOfSpecificCompositeId : this->Internal->CompositeIDs)
      {
        source->SetContentType(nodeId, vtkSelectionNode::INDICES);
        source->SetContainingCells(nodeId, this->ContainingCells);
        source->SetInverse(nodeId, this->Inverse);
        source->SetNumberOfLayers(nodeId, this->NumberOfLayers);
        source->SetRemoveSeed(this->RemoveSeed);
        source->SetRemoveIntermediateLayers(this->RemoveIntermediateLayers);
        source->SetCompositeIndex(nodeId, static_cast<int>(pieceIdsOfSpecificCompositeId.first));
        for (const auto& pieceId : pieceIdsOfSpecificCompositeId.second)
        {
          source->AddID(nodeId, pieceId.Piece, pieceId.ID);
        }
        ++nodeId;
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case HIERARCHICALID:
    {
      source->SetNumberOfNodes(static_cast<int>(this->Internal->HierarchicalIDs.size()));
      unsigned int nodeId = 0;
      for (const auto& idsOfSpecificHierarchicalId : this->Internal->HierarchicalIDs)
      {
        source->SetContentType(nodeId, vtkSelectionNode::INDICES);
        source->SetContainingCells(nodeId, this->ContainingCells);
        source->SetInverse(nodeId, this->Inverse);
        source->SetNumberOfLayers(nodeId, this->NumberOfLayers);
        source->SetRemoveSeed(this->RemoveSeed);
        source->SetRemoveIntermediateLayers(this->RemoveIntermediateLayers);
        source->SetHierarchicalLevel(
          nodeId, static_cast<int>(idsOfSpecificHierarchicalId.first.Level));
        source->SetHierarchicalIndex(
          nodeId, static_cast<int>(idsOfSpecificHierarchicalId.first.DataSet));
        for (const auto& id : idsOfSpecificHierarchicalId.second)
        {
          source->AddID(nodeId, -1, id);
        }
        ++nodeId;
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case THRESHOLDS:
    {
      source->SetContentType(vtkSelectionNode::THRESHOLDS);
      source->SetArrayName(this->ArrayName);
      for (size_t thresholdId = 0; thresholdId < this->Internal->Thresholds.size();
           thresholdId += 2)
      {
        const double* threshold = &this->Internal->Thresholds[thresholdId];
        source->AddThreshold(threshold[0], threshold[1]);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case LOCATIONS:
    {
      source->SetContentType(vtkSelectionNode::LOCATIONS);
      for (size_t locationId = 0; locationId < this->Internal->Locations.size(); locationId += 3)
      {
        const double* location = &this->Internal->Locations[locationId];
        source->AddLocation(location[0], location[1], location[2]);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case BLOCKS:
    {
      source->SetContentType(vtkSelectionNode::BLOCKS);
      for (const auto& block : this->Internal->Blocks)
      {
        source->AddBlock(block);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case BLOCK_SELECTORS:
    {
      source->SetContentType(vtkSelectionNode::BLOCK_SELECTORS);
      source->SetArrayName(this->ArrayName);
      for (const auto& selector : this->Internal->BlockSelectors)
      {
        source->AddBlockSelector(selector.c_str());
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case QUERY:
    {
      source->SetContentType(vtkSelectionNode::QUERY);
      source->SetQueryString(this->QueryString);
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case VALUES:
    {
      source->SetContentType(vtkSelectionNode::VALUES);
      source->SetArrayName(this->ArrayName);
      for (const auto& value : this->Internal->Values)
      {
        source->AddID(value.Piece, value.ID);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case ID:
    default:
    {
      source->SetContentType(vtkSelectionNode::INDICES);
      for (const auto& idType : this->Internal->IDs)
      {
        source->AddID(idType.Piece, idType.ID);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode: " << this->Mode << endl;
  os << indent << "ProcessID: " << this->ProcessID << endl;
  os << indent << "FieldType: " << this->FieldType << endl;
  os << indent << "ContainingCells: " << this->ContainingCells << endl;
  os << indent << "Inverse: " << this->Inverse << endl;
  os << indent << "Frustum: " << endl;
  for (int i = 0; i < 32; i += 4)
  {
    os << indent << indent << this->Frustum[i] << ", " << this->Frustum[i + 1] << ", "
       << this->Frustum[i + 2] << ", " << this->Frustum[i + 3] << endl;
  }
  os << indent << "ArrayName: " << (this->ArrayName ? this->ArrayName : "(null)") << endl;
  os << indent << "QueryString: " << (this->QueryString ? this->QueryString : "(null)") << endl;
  os << indent << "NumberOfLayers: " << this->NumberOfLayers << endl;
  os << indent << "RemoveSeed: " << this->RemoveSeed << endl;
  os << indent << "RemoveIntermediateLayers: " << this->RemoveIntermediateLayers << endl;
}
