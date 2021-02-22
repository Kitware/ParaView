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
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSelectionSource.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#include <set>
#include <vector>

class vtkPVSelectionSource::vtkInternal
{
public:
  struct IDType
  {
    vtkIdType Piece;
    vtkIdType ID;
    IDType(vtkIdType piece, vtkIdType id)
    {
      this->Piece = piece;
      this->ID = id;
    }

    bool operator<(const IDType& other) const
    {
      if (this->Piece == other.Piece)
      {
        return (this->ID < other.ID);
      }
      return (this->Piece < other.Piece);
    }
  };

  struct CompositeIDType
  {
    unsigned int CompositeIndex;
    vtkIdType Piece;
    vtkIdType ID;
    CompositeIDType(unsigned int ci, vtkIdType piece, vtkIdType id)
    {
      this->CompositeIndex = ci;
      this->Piece = piece;
      this->ID = id;
    }
    bool operator<(const CompositeIDType& other) const
    {
      if (this->CompositeIndex == other.CompositeIndex)
      {
        if (this->Piece == other.Piece)
        {
          return (this->ID < other.ID);
        }
        return (this->Piece < other.Piece);
      }
      return (this->CompositeIndex < other.CompositeIndex);
    }
  };

  struct HierarchicalIDType
  {
    unsigned int Level;
    unsigned int DataSet;
    vtkIdType ID;
    HierarchicalIDType(unsigned int level, unsigned int ds, vtkIdType id)
    {
      this->Level = level;
      this->DataSet = ds;
      this->ID = id;
    }
    bool operator<(const HierarchicalIDType& other) const
    {
      if (this->Level == other.Level)
      {
        if (this->DataSet == other.DataSet)
        {
          return (this->ID < other.ID);
        }
        return (this->DataSet < other.DataSet);
      }
      return (this->Level < other.Level);
    }
  };

  struct PedigreeIDType
  {
    std::string Domain;
    vtkIdType ID;
    PedigreeIDType(std::string domain, vtkIdType id)
    {
      this->Domain = domain;
      this->ID = id;
    }
    bool operator<(const PedigreeIDType& other) const
    {
      if (this->Domain == other.Domain)
      {
        return (this->ID < other.ID);
      }
      return (this->Domain < other.Domain);
    }
  };

  struct PedigreeStringIDType
  {
    std::string Domain;
    std::string ID;
    PedigreeStringIDType(std::string domain, std::string id)
    {
      this->Domain = domain;
      this->ID = id;
    }
    bool operator<(const PedigreeStringIDType& other) const
    {
      if (this->Domain == other.Domain)
      {
        return (this->ID < other.ID);
      }
      return (this->Domain < other.Domain);
    }
  };

  typedef std::set<vtkIdType> SetOfIDs;
  typedef std::set<IDType> SetOfIDType;
  typedef std::set<CompositeIDType> SetOfCompositeIDType;
  typedef std::set<HierarchicalIDType> SetOfHierarchicalIDType;
  typedef std::set<PedigreeIDType> SetOfPedigreeIDType;
  typedef std::set<PedigreeStringIDType> SetOfPedigreeStringIDType;
  typedef std::vector<double> VectorOfDoubles;

  SetOfIDs GlobalIDs;
  SetOfIDs Blocks;
  SetOfIDType IDs;
  SetOfIDType Values;
  SetOfCompositeIDType CompositeIDs;
  SetOfHierarchicalIDType HierarchicalIDs;
  SetOfPedigreeIDType PedigreeIDs;
  SetOfPedigreeStringIDType PedigreeStringIDs;
  VectorOfDoubles Locations;
  VectorOfDoubles Thresholds;
};

vtkStandardNewMacro(vtkPVSelectionSource);
//----------------------------------------------------------------------------
vtkPVSelectionSource::vtkPVSelectionSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Internal = new vtkInternal();
  this->Mode = ID;
  this->ContainingCells = 1;
  this->Inverse = 0;
  this->ArrayName = nullptr;
  for (int cc = 0; cc < 32; cc++)
  {
    this->Frustum[cc] = 0;
  }
  this->FieldType = vtkSelectionNode::CELL;
  this->NumberOfLayers = 0;
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
  this->Internal->PedigreeIDs.insert(vtkInternal::PedigreeIDType(domain, id));
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
  this->Internal->PedigreeStringIDs.insert(vtkInternal::PedigreeStringIDType(domain, id));
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
  this->Internal->IDs.insert(vtkInternal::IDType(piece, id));
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
  this->Internal->Values.insert(vtkInternal::IDType(piece, value));
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
  this->Internal->CompositeIDs.insert(vtkInternal::CompositeIDType(composite_index, piece, id));
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
  this->Internal->HierarchicalIDs.insert(vtkInternal::HierarchicalIDType(level, dataset, id));
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
void vtkPVSelectionSource::SetArrayName(const char* arrayName)
{
  if (this->ArrayName == nullptr && arrayName == nullptr)
  {
    return;
  }

  if (this->ArrayName && arrayName && strcmp(this->ArrayName, arrayName) == 0)
  {
    return;
  }

  delete[] this->ArrayName;
  this->ArrayName = nullptr;
  if (arrayName)
  {
    size_t n = strlen(arrayName) + 1;
    char* cp1 = new char[n];
    const char* cp2 = (arrayName);
    this->ArrayName = cp1;
    do
    {
      *cp1++ = *cp2++;
    } while (--n);
  }

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

  vtkSelectionSource* source = vtkSelectionSource::New();
  source->SetFieldType(this->FieldType);
  source->SetContainingCells(this->ContainingCells);
  source->SetInverse(this->Inverse);
  source->SetNumberOfLayers(this->NumberOfLayers);

  switch (this->Mode)
  {
    case FRUSTUM:
      source->SetContentType(vtkSelectionNode::FRUSTUM);
      source->SetFrustum(this->Frustum);
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
      break;

    case GLOBALIDS:
    {
      source->SetContentType(vtkSelectionNode::GLOBALIDS);
      source->RemoveAllIDs();
      vtkInternal::SetOfIDs::iterator iter;
      for (iter = this->Internal->GlobalIDs.begin(); iter != this->Internal->GlobalIDs.end();
           ++iter)
      {
        source->AddID(-1, *iter);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case PEDIGREEIDS:
    {
      source->SetContentType(vtkSelectionNode::INDICES);

      // Add integer IDs
      source->RemoveAllIDs();
      vtkInternal::SetOfPedigreeIDType::iterator iter;
      for (iter = this->Internal->PedigreeIDs.begin(); iter != this->Internal->PedigreeIDs.end();
           ++iter)
      {
        if (iter == this->Internal->PedigreeIDs.begin() || !source->GetArrayName() ||
          source->GetArrayName() != iter->Domain)
        {
          if (iter != this->Internal->PedigreeIDs.begin())
          {
            source->UpdatePiece(piece, npieces, 0);
            vtkSelectionNode* clone = vtkSelectionNode::New();
            clone->ShallowCopy(source->GetOutput()->GetNode(0));
            clone->SetContentType(vtkSelectionNode::PEDIGREEIDS);
            output->AddNode(clone);
            clone->Delete();
          }

          source->RemoveAllIDs();
          source->SetArrayName(iter->Domain.c_str());
        }
        source->AddID(-1, iter->ID);
      }
      if (this->Internal->PedigreeIDs.size() > 0)
      {
        source->UpdatePiece(piece, npieces, 0);
        vtkSelectionNode* clone = vtkSelectionNode::New();
        clone->ShallowCopy(source->GetOutput()->GetNode(0));
        clone->SetContentType(vtkSelectionNode::PEDIGREEIDS);
        output->AddNode(clone);
        clone->Delete();
      }

      // Add string IDs
      source->RemoveAllStringIDs();
      vtkInternal::SetOfPedigreeStringIDType::iterator siter;
      for (siter = this->Internal->PedigreeStringIDs.begin();
           siter != this->Internal->PedigreeStringIDs.end(); ++siter)
      {
        if (siter == this->Internal->PedigreeStringIDs.begin() || !source->GetArrayName() ||
          source->GetArrayName() != siter->Domain)
        {
          if (siter != this->Internal->PedigreeStringIDs.begin())
          {
            source->UpdatePiece(piece, npieces, 0);
            vtkSelectionNode* clone = vtkSelectionNode::New();
            clone->ShallowCopy(source->GetOutput()->GetNode(0));
            clone->SetContentType(vtkSelectionNode::PEDIGREEIDS);
            output->AddNode(clone);
            clone->Delete();
          }

          source->RemoveAllIDs();
          source->SetArrayName(siter->Domain.c_str());
        }
        source->AddStringID(-1, siter->ID.c_str());
      }
      if (this->Internal->PedigreeStringIDs.size() > 0)
      {
        source->UpdatePiece(piece, npieces, 0);
        vtkSelectionNode* clone = vtkSelectionNode::New();
        clone->ShallowCopy(source->GetOutput()->GetNode(0));
        clone->SetContentType(vtkSelectionNode::PEDIGREEIDS);
        output->AddNode(clone);
        clone->Delete();
      }
    }
    break;

    case COMPOSITEID:
    {
      source->SetContentType(vtkSelectionNode::INDICES);
      vtkInternal::SetOfCompositeIDType::iterator iter;
      for (iter = this->Internal->CompositeIDs.begin(); iter != this->Internal->CompositeIDs.end();
           ++iter)
      {
        if (iter == this->Internal->CompositeIDs.begin() ||
          source->GetCompositeIndex() != static_cast<int>(iter->CompositeIndex))
        {
          if (iter != this->Internal->CompositeIDs.begin())
          {
            source->UpdatePiece(piece, npieces, 0);
            vtkSelectionNode* clone = vtkSelectionNode::New();
            clone->ShallowCopy(source->GetOutput()->GetNode(0));
            output->AddNode(clone);
            clone->Delete();
          }

          source->RemoveAllIDs();
          source->SetCompositeIndex(static_cast<int>(iter->CompositeIndex));
        }
        source->AddID(iter->Piece, iter->ID);
      }
      if (this->Internal->CompositeIDs.size() > 0)
      {
        source->UpdatePiece(piece, npieces, 0);
        vtkSelectionNode* clone = vtkSelectionNode::New();
        clone->ShallowCopy(source->GetOutput()->GetNode(0));
        output->AddNode(clone);
        clone->Delete();
      }
    }
    break;

    case HIERARCHICALID:
    {
      source->SetContentType(vtkSelectionNode::INDICES);
      vtkInternal::SetOfHierarchicalIDType::iterator iter;
      for (iter = this->Internal->HierarchicalIDs.begin();
           iter != this->Internal->HierarchicalIDs.end(); ++iter)
      {
        if (source->GetHierarchicalLevel() != static_cast<int>(iter->Level) ||
          source->GetHierarchicalIndex() != static_cast<int>(iter->DataSet))
        {
          if (iter != this->Internal->HierarchicalIDs.begin())
          {
            source->UpdatePiece(piece, npieces, 0);
            vtkSelectionNode* clone = vtkSelectionNode::New();
            clone->ShallowCopy(source->GetOutput()->GetNode(0));
            output->AddNode(clone);
            clone->Delete();
          }

          source->RemoveAllIDs();
          source->SetHierarchicalLevel(static_cast<int>(iter->Level));
          source->SetHierarchicalIndex(static_cast<int>(iter->DataSet));
        }
        source->AddID(-1, iter->ID);
      }
      if (this->Internal->HierarchicalIDs.size() > 0)
      {
        source->UpdatePiece(piece, npieces, 0);
        vtkSelectionNode* clone = vtkSelectionNode::New();
        clone->ShallowCopy(source->GetOutput()->GetNode(0));
        output->AddNode(clone);
        clone->Delete();
      }
    }
    break;

    case THRESHOLDS:
    {
      source->SetContentType(vtkSelectionNode::THRESHOLDS);
      source->SetArrayName(this->ArrayName);
      vtkInternal::VectorOfDoubles::iterator iter;
      for (iter = this->Internal->Thresholds.begin(); iter != this->Internal->Thresholds.end();)
      {
        double min = *iter;
        iter++;
        double max = *iter;
        iter++;
        source->AddThreshold(min, max);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case LOCATIONS:
    {
      source->SetContentType(vtkSelectionNode::LOCATIONS);
      vtkInternal::VectorOfDoubles::iterator iter;
      for (iter = this->Internal->Locations.begin(); iter != this->Internal->Locations.end();)
      {
        double x = *iter;
        iter++;
        double y = *iter;
        iter++;
        double z = *iter;
        iter++;
        source->AddLocation(x, y, z);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case BLOCKS:
    {
      source->SetContentType(vtkSelectionNode::BLOCKS);
      vtkInternal::SetOfIDs::iterator iter;
      for (iter = this->Internal->Blocks.begin(); iter != this->Internal->Blocks.end(); ++iter)
      {
        source->AddBlock(*iter);
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
      source->RemoveAllIDs();
      vtkInternal::SetOfIDType::iterator iter;
      for (iter = this->Internal->Values.begin(); iter != this->Internal->Values.end(); ++iter)
      {
        source->AddID(iter->Piece, iter->ID);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;

    case ID:
    default:
    {
      source->SetContentType(vtkSelectionNode::INDICES);
      source->RemoveAllIDs();
      vtkInternal::SetOfIDType::iterator iter;
      for (iter = this->Internal->IDs.begin(); iter != this->Internal->IDs.end(); ++iter)
      {
        source->AddID(iter->Piece, iter->ID);
      }
      source->UpdatePiece(piece, npieces, 0);
      output->ShallowCopy(source->GetOutput());
    }
    break;
  }
  source->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSelectionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfLayers: " << this->NumberOfLayers << endl;
}
