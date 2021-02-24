/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCTHSource.h"
#include "vtkAMRBox.h"
#include "vtkBoundingBox.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCTHDataArray.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

//---------------------------------------------------------------------------
vtkCTHSource::vtkCTHSource() = default;

vtkCTHSource::~vtkCTHSource()
{
  if (NeighborArray)
  {
    NeighborArray->Delete();
    NeighborArray = nullptr;
  }
  for (size_t i = 0; i < this->Blocks.size(); i++)
  {
    Block& b = this->Blocks[i];
    if (b.allocated)
    {
      b.ug->Delete();
      delete b.box;
    }
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::Initialize(int vtkNotUsed(igm), int n_blocks, int nmat, int vtkNotUsed(max_mat),
  int NCFieldNames, int NMFieldNames, double* min, double* max, int max_level)
{
  this->MaxLevel = max_level;
  this->Bounds.SetMinPoint(min);
  this->Bounds.SetMaxPoint(max);
  this->CFieldNames.resize(NCFieldNames);
  this->MFieldNames.resize(nmat);

  for (int i = 0; i < nmat; i++)
  {
    this->MFieldNames[i].resize(NMFieldNames);
  }

  this->Blocks.clear();
  this->Blocks.resize(n_blocks);

  // These are set during block initialization
  this->MinLevel = this->MaxLevel + 1;
  this->GlobalBlockSize[0] = 0;
  this->GlobalBlockSize[1] = 0;
  this->GlobalBlockSize[2] = 0;

  this->NeighborArray = vtkIntArray::New();
  this->NeighborArray->SetNumberOfComponents(1);
  this->NeighborArray->SetName("Neighbors");
}

//---------------------------------------------------------------------------
void vtkCTHSource::SetCellFieldName(
  int field_id, char* field_name, char* comment, int vtkNotUsed(matid))
{
  if (strncmp(field_name, "P ", 2) && strncmp(field_name, "T ", 2) &&
    strncmp(field_name, "VX ", 3) && strncmp(field_name, "VY ", 3) && strncmp(field_name, "VZ ", 3))
  {
    CFieldNames[field_id] = std::string();
  }
  else
  {
    // strip off the trailing "!" from the comment
    char* end = strchr(comment, '!');
    CFieldNames[field_id] = std::string(comment, end - comment);
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::SetMaterialFieldName(int field_id, char* field_name, char* comment)
{
  size_t mats = this->MFieldNames.size();
  if (strncmp(field_name, "VOLM ", 5) && strncmp(field_name, "M ", 2))
  // if (strncmp (field_name, "VOLM ", 5))
  {
    std::string s;
    for (size_t i = 0; i < mats; i++)
    {
      MFieldNames[i][field_id] = s;
    }
  }
  else
  {
    for (size_t i = 0; i < mats; i++)
    {
      // strip off the trailing "!" from the comment
      char* end = strchr(comment, '!');
      std::string s(comment, end - comment);
      char n[13];
      sprintf(n, " - %zu", i + 1);
      s.append(n);
      MFieldNames[i][field_id] = s;
    }
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::SetCellFieldPointer(int block_id, int field_id, int k, int j, double* istrip)
{
  Block& b = this->Blocks[block_id];
  if (b.CFieldData[field_id] != nullptr)
  {
    /// TODO fix for multiple components
    b.CFieldData[field_id]->SetDataPointer(0, k, j, istrip);
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::SetMaterialFieldPointer(
  int block_id, int field_id, int mat, int k, int j, double* istrip)
{
  Block& b = this->Blocks[block_id];
  if (b.MFieldData[mat][field_id] != nullptr)
  {
    /// TODO fix for multiple components
    b.MFieldData[mat][field_id]->SetDataPointer(0, k, j, istrip);
    if (b.actualMaterials <= mat)
    {
      b.actualMaterials = mat + 1;
    }
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::InitializeBlock(int block_id, int Nx, int Ny, int Nz, double* x, double* y,
  double* z, int allocated, int active, int level)
{
  Block& b = this->Blocks[block_id];
  b.id = block_id;
  b.allocated = allocated;
  b.active = active;
  b.level = level;
  b.ug = nullptr;
  b.Nx = Nx;
  b.Ny = Ny;
  b.Nz = Nz;
  b.x = x;
  b.y = y;
  b.z = z;
  // set the placeholders for the strips passed in by scx/smx
  size_t numCFields = this->CFieldNames.size();
  b.CFieldData.resize(numCFields);
  for (size_t c = 0; c < numCFields; c++)
  {
    if (!this->CFieldNames[c].empty())
    {
      b.CFieldData[c] = vtkSmartPointer<vtkCTHDataArray>::New();
      b.CFieldData[c]->SetName(this->CFieldNames[c].c_str());
      b.CFieldData[c]->SetDimensions(b.Nx, b.Ny, b.Nz);
    }
    else
    {
      b.CFieldData[c] = nullptr;
    }
  }
  b.actualMaterials = 0; // so far we've filled none
  size_t numMFields = this->MFieldNames.size();
  b.MFieldData.resize(numMFields);
  for (size_t m = 0; m < numMFields; m++)
  {
    size_t numFields = this->MFieldNames[m].size();
    b.MFieldData[m].resize(numFields);
    for (size_t f = 0; f < numFields; f++)
    {
      if (!this->MFieldNames[m][f].empty())
      {
        b.MFieldData[m][f] = vtkSmartPointer<vtkCTHDataArray>::New();
        b.MFieldData[m][f]->SetName(this->MFieldNames[m][f].c_str());
        b.MFieldData[m][f]->SetDimensions(b.Nx, b.Ny, b.Nz);
      }
      else
      {
        b.MFieldData[m][f] = nullptr;
      }
    }
  }

  this->AllocationsChanged = true;

  if (this->GlobalBlockSize[0] == 0 && this->GlobalBlockSize[1] == 0 &&
    this->GlobalBlockSize[2] == 0)
  {
    this->GlobalBlockSize[0] = Nx;
    this->GlobalBlockSize[1] = Ny;
    this->GlobalBlockSize[2] = Nz;
  }
  else
  {
    if (this->GlobalBlockSize[0] != -1 && this->GlobalBlockSize[0] != Nx)
    {
      this->GlobalBlockSize[0] = -1;
    }
    if (this->GlobalBlockSize[1] != -1 && this->GlobalBlockSize[1] != Ny)
    {
      this->GlobalBlockSize[1] = -1;
    }
    if (this->GlobalBlockSize[2] != -1 && this->GlobalBlockSize[2] != Nz)
    {
      this->GlobalBlockSize[2] = -1;
    }
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::UpdateBlock(int block_id, int allocated, int active, int level, int max_level,
  int bxbot, int bxtop, int bybot, int bytop, int bzbot, int bztop, int* neighb_proc,
  int* neighb_block)
{
  Block& b = this->Blocks[block_id];
  b.active = active;
  b.bot[0] = bxbot;
  b.bot[1] = bybot;
  b.bot[2] = bzbot;
  b.top[0] = bxtop;
  b.top[1] = bytop;
  b.top[2] = bztop;

  memcpy(b.neighbor_proc, neighb_proc, 24 * sizeof(int));
  memcpy(b.neighbor_block, neighb_block, 24 * sizeof(int));

  if (b.allocated != allocated)
  {
    b.allocated = allocated;
    this->AllocationsChanged = true;
  }
  b.level = level;
  this->MaxLevel = max_level;

  if (b.allocated && b.level < this->MinLevel)
  {
    this->MinLevel = b.level;
    this->MinLevelSpacing[0] = b.x[1] - b.x[0];
    this->MinLevelSpacing[1] = b.y[1] - b.y[0];
    this->MinLevelSpacing[2] = b.z[1] - b.z[0];
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::UpdateRepresentation()
{
  // A call to viz implies that our data is modified.
  for (size_t i = 0; i < Blocks.size(); i++)
  {
    Block& b = this->Blocks[i];
    if (!b.allocated)
    {
      continue;
    }
    for (size_t c = 0; c < b.CFieldData.size(); c++)
    {
      if (b.CFieldData[c] == nullptr)
        continue;
      b.CFieldData[c]->Modified();
      /*
            vtkDataArray* da = b.ug->GetCellData ()->GetArray (b.CFieldData[c]->GetName ());
            if (da)
              {
              int len = b.CFieldData[c]->GetNumberOfTuples ();
              for (int i = 0; i < len; i ++)
                {
                double *tup = b.CFieldData[c]->GetTuple (i);
                da->SetComponent (i, 0, tup[0]);
                }
              }
      */
    }
    for (size_t m = 0; m < b.MFieldData.size(); m++)
    {
      for (size_t f = 0; f < b.MFieldData[m].size(); f++)
      {
        if (b.MFieldData[m][f] == nullptr)
          continue;
        if (strncmp(b.MFieldData[m][f]->GetName(), "Volume Fraction", 15) != 0)
        {
          b.MFieldData[m][f]->Modified();
          /*
                    vtkDataArray* da = b.ug->GetCellData ()->GetArray (b.MFieldData[m][f]->GetName
             ());
                    if (da)
                      {
                      int len = b.MFieldData[m][f]->GetNumberOfTuples ();
                      for (int i = 0; i < len; i ++)
                        {
                        double *tup = b.MFieldData[m][f]->GetTuple (i);
                        da->SetComponent (i, 0, tup[0]);
                        }
                      }
          */
        }
        else
        {
          vtkDataArray* da = b.ug->GetCellData()->GetArray(b.MFieldData[m][f]->GetName());
          if (da)
          {
            int len = b.MFieldData[m][f]->GetNumberOfTuples();
            for (int idx = 0; idx < len; idx++)
            {
              double* tup = b.MFieldData[m][f]->GetTuple(idx);
              da->SetComponent(idx, 0, tup[0] * 255.0);
            }
          }
        }
      }
    }
  }
  this->AMRSet->Modified();
}

//---------------------------------------------------------------------------
int vtkCTHSource::FillInputData(vtkCPInputDataDescription* input)
{
  if (this->AllocationsChanged)
  {
    this->AMRSet = vtkSmartPointer<vtkNonOverlappingAMR>::New();
    input->SetGrid(this->AMRSet);

    std::vector<int> datasets(this->MaxLevel + 1, 0);
    // clear out the hierarchy
    for (size_t i = 0; i < Blocks.size(); i++)
    {
      Block& b = this->Blocks[i];
      datasets[b.level]++;
    }
    AMRSet->Initialize(this->MaxLevel + 1, &datasets[0]);

    this->NeighborArray->SetNumberOfTuples(0);
    for (size_t i = 0; i < Blocks.size(); i++)
    {
      Block& b = this->Blocks[i];
      if (b.ug)
      {
        b.ug->Delete();
        b.ug = nullptr;
        delete b.box;
      }
      if (b.allocated)
      {
        AllocateBlock(b);
        datasets[b.level]--;
        AMRSet->SetDataSet(b.level, datasets[b.level], b.ug);
      }
    }

    this->AddAttributesToAMR(AMRSet);
    // this->AllocationsChanged = false;
  }
  UpdateRepresentation();

  return 1;
}

//---------------------------------------------------------------------------
void vtkCTHSource::AllocateBlock(Block& b)
{
  int loCorner[3];
  int hiCorner[3];

  bool extentsChanged = this->GetBounds(b, loCorner, hiCorner);

  b.box = new vtkAMRBox(loCorner, hiCorner);

  int dx = hiCorner[0] - loCorner[0] + 1;
  int dy = hiCorner[1] - loCorner[1] + 1;
  int dz = hiCorner[2] - loCorner[2] + 1;

  b.ug = vtkUniformGrid::New();
  b.ug->SetExtent(0, dx, 0, dy, 0, dz);
  b.ug->SetSpacing(b.x[1] - b.x[0], b.y[1] - b.y[0], b.z[1] - b.z[0]);
  b.ug->SetOrigin(b.x[loCorner[0]], b.y[loCorner[1]], b.z[loCorner[2]]);

  this->AddGhostArray(b, dx, dy, dz);
  this->AddBlockIdArray(b, dx, dy, dz);
  this->AddNeighborArray(b);

  // Need to add this since if we do Cutter first ,
  // it no longer looks hierarchical to PVGeomFilter
  // this->AddAMRLevelArray (b, dx, dy, dz);

  if (extentsChanged)
  {
    this->AddFieldArrays(b, loCorner, hiCorner);
    // this->AddFieldArrays (b);
  }
  else
  {
    this->AddFieldArrays(b);
  }

  // this->AddActivationArray (b);
}

//---------------------------------------------------------------------------
bool vtkCTHSource::GetBounds(Block& b, int loCorner[3], int hiCorner[3])
{
  loCorner[0] = 0;
  loCorner[1] = 0;
  loCorner[2] = 0;
  hiCorner[0] = b.Nx - 1;
  hiCorner[1] = b.Ny - 1;
  hiCorner[2] = b.Nz - 1;
  const double* min = this->Bounds.GetMinPoint();
  const double* max = this->Bounds.GetMaxPoint();
  bool extentsChanged = false;
  // CTH allows the blocks to extent one cell outside the real bounds
  // Normally for blocks these extra cells are ghost cells,
  // but outside the simulation bounds, we don't want to viz them at all
  if (b.x[0] < min[0])
  {
    loCorner[0]++;
    extentsChanged = true;
  }
  if (b.x[b.Nx - 1] > max[0])
  {
    hiCorner[0]--;
    extentsChanged = true;
  }
  if (b.y[0] < min[1])
  {
    loCorner[1]++;
    extentsChanged = true;
  }
  if (b.y[b.Ny - 1] > max[1])
  {
    hiCorner[1]--;
    extentsChanged = true;
  }
  if (b.z[0] < min[2])
  {
    loCorner[2]++;
    extentsChanged = true;
  }
  if (b.z[b.Nz - 1] > max[2])
  {
    hiCorner[2]--;
    extentsChanged = true;
  }
  // if they're changed the CTHDataArray needs to know about it
  return extentsChanged;
}

//---------------------------------------------------------------------------
void vtkCTHSource::AddGhostArray(Block& b, int dx, int dy, int dz)
{
  vtkCellData* cd = b.ug->GetCellData();

  // The outside edge cells (not global bounds) are ghost cells level 1
  vtkUnsignedCharArray* ghostArray = vtkUnsignedCharArray::New();
  ghostArray->SetNumberOfTuples(dx * dy * dz);
  ghostArray->SetName(vtkDataSetAttributes::GhostArrayName());
  cd->AddArray(ghostArray);
  ghostArray->Delete();

  int ghostsLower[3];
  int ghostsUpper[3];
  for (int i = 0; i < 3; i++)
  {
    ghostsLower[i] = (b.active && b.bot[i] < 0);
    ghostsUpper[i] = (b.active && b.top[i] < 0);
  }

  unsigned char* ptr = static_cast<unsigned char*>(ghostArray->GetVoidPointer(0));
  for (int k = 0; k < dz; k++)
  {
    if ((dz > 1) && ((ghostsLower[2] && (k == 0)) || (ghostsUpper[2] && (k == (dz - 1)))))
    {
      memset(ptr, vtkDataSetAttributes::DUPLICATECELL, dx * dy);
      ptr += dx * dy;
      continue;
    }
    for (int j = 0; j < dy; j++)
    {
      if ((dy > 1) && ((ghostsLower[1] && (j == 0)) || (ghostsUpper[1] && (j == (dy - 1)))))
      {
        memset(ptr, vtkDataSetAttributes::DUPLICATECELL, dx);
      }
      else
      {
        memset(ptr, 0, dx);
        if (dx > 1)
        {
          if (ghostsLower[0])
          {
            ptr[0] = 1;
          }
          if (ghostsUpper[0])
          {
            ptr[dx - 1] = 1;
          }
        }
      }
      ptr += dx;
    }
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::AddBlockIdArray(Block& b, int dx, int dy, int dz)
{
  vtkCellData* cd = b.ug->GetCellData();

  vtkIntArray* blockArray = vtkIntArray::New();
  blockArray->SetNumberOfTuples(dx * dy * dz);
  blockArray->SetName("ElementBlockIds");
  cd->AddArray(blockArray);
  blockArray->Delete();

  int* ptr = static_cast<int*>(blockArray->GetVoidPointer(0));
  memset(ptr, b.id, dx * dy * dz);
}

void vtkCTHSource::AddNeighborArray(Block& b)
{
  vtkMultiProcessController* ctrl = vtkMultiProcessController::GetGlobalController();
  int localId = ctrl->GetLocalProcessId();

  int len = 24; // known quantity passed in from Adaptor
  for (int i = 0; i < len; i++)
  {
    if (b.neighbor_block[i] == 0 || b.neighbor_proc[i] == localId)
    {
      continue;
    }
    bool present = false;
    for (int j = 0; j < NeighborArray->GetNumberOfTuples(); j++)
    {
      int compare;
      NeighborArray->GetTypedTuple(j, &compare);
      if (b.neighbor_proc[i] == compare)
      {
        present = true;
        break;
      }
    }
    if (!present)
    {
      NeighborArray->InsertNextTypedTuple(&(b.neighbor_proc[i]));
    }
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::AddAMRLevelArray(Block& b, int dx, int dy, int dz)
{
  vtkUnsignedCharArray* amrLevel = vtkUnsignedCharArray::New();
  amrLevel->SetNumberOfTuples(dx * dy * dz);
  amrLevel->SetName("vtkAMRLevel");
  memset(amrLevel->GetVoidPointer(0), b.level, sizeof(unsigned char) * dx * dy * dz);
  b.ug->GetCellData()->AddArray(amrLevel);
  amrLevel->Delete();
}

//---------------------------------------------------------------------------
void vtkCTHSource::AddFieldArrays(Block& b)
{
  vtkCellData* cd = b.ug->GetCellData();

  for (size_t c = 0; c < b.CFieldData.size(); c++)
  {
    if (b.CFieldData[c] == nullptr)
      continue;
    b.CFieldData[c]->UnsetExtents();
    cd->AddArray(b.CFieldData[c]);
    /*
        vtkDoubleArray* copied = vtkDoubleArray::New ();
        copied->SetName (b.CFieldData[c]->GetName ());
        copied->SetNumberOfComponents (1);
        copied->SetNumberOfTuples (b.CFieldData[c]->GetNumberOfTuples ());
        cd->AddArray (copied);
        copied->Delete ();
    */
  }
  for (int m = 0; m < b.actualMaterials; m++)
  {
    // TODO In general for in situ, we should only pull the data we'd actually want.
    for (size_t f = 0; f < b.MFieldData[m].size(); f++)
    {
      if (b.MFieldData[m][f] == nullptr)
        continue;
      b.MFieldData[m][f]->UnsetExtents();
      if (strncmp(b.MFieldData[m][f]->GetName(), "Volume Fraction", 15) != 0)
      {
        cd->AddArray(b.MFieldData[m][f]);
        /*
                vtkDoubleArray* copied = vtkDoubleArray::New ();
                copied->SetName (b.MFieldData[m][f]->GetName ());
                copied->SetNumberOfComponents (1);
                copied->SetNumberOfTuples (b.MFieldData[m][f]->GetNumberOfTuples ());
                cd->AddArray (copied);
                copied->Delete ();
        */
      }
      else
      {
        vtkIntArray* rounded = vtkIntArray::New();
        rounded->SetName(b.MFieldData[m][f]->GetName());
        rounded->SetNumberOfComponents(1);
        rounded->SetNumberOfTuples(b.MFieldData[m][f]->GetNumberOfTuples());
        cd->AddArray(rounded);
        rounded->Delete();
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::AddFieldArrays(Block& b, int loCorner[3], int hiCorner[3])
{
  vtkCellData* cd = b.ug->GetCellData();

  for (size_t c = 0; c < b.CFieldData.size(); c++)
  {
    if (b.CFieldData[c] == nullptr)
      continue;
    b.CFieldData[c]->SetExtents(loCorner, hiCorner);
    cd->AddArray(b.CFieldData[c]);
    /*
        vtkDoubleArray* copied = vtkDoubleArray::New ();
        copied->SetName (b.CFieldData[c]->GetName ());
        copied->SetNumberOfComponents (1);
        copied->SetNumberOfTuples (b.CFieldData[c]->GetNumberOfTuples ());
        cd->AddArray (copied);
        copied->Delete ();
    */
  }
  for (int m = 0; m < b.actualMaterials; m++)
  {
    // TODO In general for in situ, we should only pull the data we'd actually want.
    for (size_t f = 0; f < b.MFieldData[m].size(); f++)
    {
      if (b.MFieldData[m][f] == nullptr)
        continue;
      b.MFieldData[m][f]->SetExtents(loCorner, hiCorner);
      if (strncmp(b.MFieldData[m][f]->GetName(), "Volume Fraction", 15) != 0)
      {
        cd->AddArray(b.MFieldData[m][f]);
        /*
                vtkDoubleArray* copied = vtkDoubleArray::New ();
                copied->SetName (b.MFieldData[m][f]->GetName ());
                copied->SetNumberOfComponents (1);
                copied->SetNumberOfTuples (b.MFieldData[m][f]->GetNumberOfTuples ());
                cd->AddArray (copied);
                copied->Delete ();
        */
      }
      else
      {
        vtkIntArray* rounded = vtkIntArray::New();
        rounded->SetName(b.MFieldData[m][f]->GetName());
        rounded->SetNumberOfComponents(1);
        rounded->SetNumberOfTuples(b.MFieldData[m][f]->GetNumberOfTuples());
        cd->AddArray(rounded);
        rounded->Delete();
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkCTHSource::AddActivationArray(Block& b)
{
  vtkCellData* cd = b.ug->GetCellData();
  vtkDataArray* da = cd->GetArray("ActiveBlock");
  if (!da)
  {
    da = vtkUnsignedCharArray::New();
    da->SetName("ActiveBlock");
    da->SetNumberOfTuples(b.ug->GetNumberOfCells());
    cd->AddArray(da);
    da->Delete();
  }
  da->FillComponent(0, b.active);
}

//---------------------------------------------------------------------------
void vtkCTHSource::AddAttributesToAMR(vtkNonOverlappingAMR* amr)
{
  vtkFieldData* fd = amr->GetFieldData();

  double b[6];
  this->Bounds.GetBounds(b);
  vtkDoubleArray* bounds = vtkDoubleArray::New();
  bounds->SetNumberOfTuples(6);
  bounds->SetName("GlobalBounds");
  for (int i = 0; i < 6; i++)
  {
    bounds->SetValue(i, b[i]);
  }
  fd->AddArray(bounds);
  bounds->Delete();

  vtkIntArray* boxSize = vtkIntArray::New();
  boxSize->SetNumberOfTuples(3);
  boxSize->SetName("GlobalBoxSize");
  for (int j = 0; j < 3; j++)
  {
    boxSize->SetValue(j, this->GlobalBlockSize[j]);
  }
  fd->AddArray(boxSize);
  boxSize->Delete();

  vtkIntArray* minLevel = vtkIntArray::New();
  minLevel->SetNumberOfTuples(1);
  minLevel->SetName("MinLevel");
  minLevel->SetValue(0, this->MinLevel);
  fd->AddArray(minLevel);
  minLevel->Delete();

  vtkDoubleArray* minLevelSpacing = vtkDoubleArray::New();
  minLevelSpacing->SetNumberOfTuples(3);
  minLevelSpacing->SetName("MinLevelSpacing");
  for (int k = 0; k < 3; k++)
  {
    minLevelSpacing->SetValue(k, this->MinLevelSpacing[k]);
  }
  fd->AddArray(minLevelSpacing);
  minLevelSpacing->Delete();

  if (this->NeighborArray->GetNumberOfTuples() > 0)
  {
    fd->AddArray(this->NeighborArray);
  }
}
