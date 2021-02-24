
/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpyPlotBlock.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSpyPlotBlock.h"

#include "vtkBoundingBox.h"
#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkSpyPlotIStream.h"
#include "vtkUnsignedCharArray.h"

#include <assert.h>
#include <map>
#include <sstream>

#define MinBlockBound(i) this->XYZArrays[i]->GetTuple1(0)
#define MaxBlockBound(i) this->XYZArrays[i]->GetTuple1(this->XYZArrays[i]->GetNumberOfTuples() - 1)

#define coutVector6(x)                                                                             \
  (x)[0] << " " << (x)[1] << " " << (x)[2] << " " << (x)[3] << " " << (x)[4] << " " << (x)[5]
#define coutVector3(x) (x)[0] << " " << (x)[1] << " " << (x)[2]

//-----------------------------------------------------------------------------
vtkSpyPlotBlock::vtkSpyPlotBlock()
  : Level(0)
{
  this->Level = 0;
  this->XYZArrays[0] = this->XYZArrays[1] = this->XYZArrays[2] = nullptr;
  this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] = 0;
  this->SavedExtents[0] = this->SavedExtents[2] = this->SavedExtents[4] = 1;
  this->SavedExtents[1] = this->SavedExtents[3] = this->SavedExtents[5] = 0;
  this->SavedRealExtents[0] = this->SavedRealExtents[2] = this->SavedRealExtents[4] = 1;
  this->SavedRealExtents[1] = this->SavedRealExtents[3] = this->SavedRealExtents[5] = 0;
  this->SavedRealDims[0] = this->SavedRealDims[1] = this->SavedRealDims[2] = 0;
  //
  this->Status.Active = 0;
  this->Status.Allocated = 0;
  this->Status.Fixed = 0;
  this->Status.Debug = 0;
  this->Status.AMR = 0;

  this->CoordSystem = Cartesian3D;
}

//-----------------------------------------------------------------------------
vtkSpyPlotBlock::~vtkSpyPlotBlock()
{
  if (!this->IsAllocated())
  {
    return;
  }
  this->XYZArrays[0]->Delete();
  this->XYZArrays[1]->Delete();
  this->XYZArrays[2]->Delete();
}

//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::SetDebug(unsigned char mode)
{
  if (mode)
  {
    this->Status.Debug = 1;
  }
  else
  {
    this->Status.Debug = 0;
  }
}

//-----------------------------------------------------------------------------
unsigned char vtkSpyPlotBlock::GetDebug() const
{
  return this->Status.Debug;
}

//-----------------------------------------------------------------------------
// world space
void vtkSpyPlotBlock::GetBounds(double bounds[6]) const
{
  bounds[0] = MinBlockBound(0);
  bounds[1] = MaxBlockBound(0);
  bounds[2] = MinBlockBound(1);
  bounds[3] = MaxBlockBound(1);
  bounds[4] = MinBlockBound(2);
  bounds[5] = MaxBlockBound(2);
}

//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::GetRealBounds(double rbounds[6]) const
{
  int i, j = 0;
  if (this->IsAMR())
  {
    double spacing, maxV, minV;
    for (i = 0; i < 3; i++)
    {
      if (this->Dimensions[i] > 1)
      {
        minV = MinBlockBound(i);
        maxV = MaxBlockBound(i);
        spacing = (maxV - minV) / this->Dimensions[i];
        rbounds[j++] = minV + spacing;
        rbounds[j++] = maxV - spacing;
        continue;
      }
      rbounds[j++] = 0;
      rbounds[j++] = 0;
    }
    return;
  }

  // If the block has been fixed then the XYZArrays true size is dim - 2
  // (-2 represents the original endpoints being removed) - fixOffset is
  // used to correct for this
  int fixOffset = 0;
  if (!this->IsFixed())
  {
    fixOffset = 1;
  }

  for (i = 0; i < 3; i++)
  {
    if (this->Dimensions[i] > 1)
    {
      rbounds[j++] = this->XYZArrays[i]->GetTuple1(fixOffset);
      rbounds[j++] = this->XYZArrays[i]->GetTuple1(this->Dimensions[i] + fixOffset - 2);
      continue;
    }
    rbounds[j++] = 0;
    rbounds[j++] = 0;
  }
}

//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::GetSpacing(double spacing[3]) const
{
  double maxV, minV;
  for (int q = 0; q < 3; ++q)
  {
    minV = MinBlockBound(q);
    maxV = MaxBlockBound(q);
    spacing[q] = (maxV - minV) / this->Dimensions[q];
  }
}

//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::GetVectors(vtkDataArray* coordinates[3]) const
{
  assert("Check Block is not AMR" && (!this->IsAMR()));
  coordinates[0] = this->XYZArrays[0];
  coordinates[1] = this->XYZArrays[1];
  coordinates[2] = this->XYZArrays[2];
}

//-----------------------------------------------------------------------------
int vtkSpyPlotBlock::GetAMRInformation(const vtkBoundingBox& globalBounds, int* level,
  double spacing[3], double origin[3], int extents[6], int realExtents[6], int realDims[3]) const
{
  assert("Check Block is AMR" && this->IsAMR());
  *level = this->Level;
  this->GetExtents(extents);
  int i, j, hasBadCells = 0;
  const double* minP = globalBounds.GetMinPoint();
  const double* maxP = globalBounds.GetMaxPoint();

  double minV, maxV;
  for (i = 0, j = 0; i < 3; i++, j++)
  {
    minV = MinBlockBound(i);
    maxV = MaxBlockBound(i);

    spacing[i] = (maxV - minV) / this->Dimensions[i];

    if (this->Dimensions[i] == 1)
    {
      origin[i] = 0.0;
      realExtents[j++] = 0;
      realExtents[j++] = 1; //!
      realDims[i] = 1;
      continue;
    }

    if (minV < minP[i])
    {
      realExtents[j] = 1;
      origin[i] = minV + spacing[i];
      hasBadCells = 1;
      if (!this->IsFixed())
      {
        --extents[j + 1];
      }
    }
    else
    {
      realExtents[j] = 0;
      origin[i] = minV;
    }
    ++j;
    if (maxV > maxP[i])
    {
      realExtents[j] = this->Dimensions[i] - 1;
      hasBadCells = 1;
      if (!this->IsFixed())
      {
        --extents[j];
      }
    }
    else
    {
      realExtents[j] = this->Dimensions[i];
    }
    realDims[i] = realExtents[j] - realExtents[j - 1];
  }
  return hasBadCells;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotBlock::FixInformation(const vtkBoundingBox& globalBounds, int extents[6],
  int realExtents[6], int realDims[3], vtkDataArray* ca[3])
{
  // This better not be a AMR block!
  assert("Check Block is not AMR" && (!this->IsAMR()));
  const double* minP = globalBounds.GetMinPoint();
  const double* maxP = globalBounds.GetMaxPoint();

  this->GetExtents(extents);

  if (this->Status.Fixed)
  {
    // if a previous run through removed the invalid ghost cells,
    // reuse the extent and geometry information calculated then
    extents[0] = this->SavedExtents[0];
    extents[1] = this->SavedExtents[1];
    extents[2] = this->SavedExtents[2];
    extents[3] = this->SavedExtents[3];
    extents[4] = this->SavedExtents[4];
    extents[5] = this->SavedExtents[5];
    realExtents[0] = this->SavedRealExtents[0];
    realExtents[1] = this->SavedRealExtents[1];
    realExtents[2] = this->SavedRealExtents[2];
    realExtents[3] = this->SavedRealExtents[3];
    realExtents[4] = this->SavedRealExtents[4];
    realExtents[5] = this->SavedRealExtents[5];
    realDims[0] = this->SavedRealDims[0];
    realDims[1] = this->SavedRealDims[1];
    realDims[2] = this->SavedRealDims[2];

    for (int i = 0; i < 3; i++)
    {
      if (this->Dimensions[i] == 1)
      {
        ca[i] = nullptr;
        continue;
      }
      ca[i] = this->XYZArrays[i];
    }
    return 1;
  }

  int i, j, hasBadGhostCells = 0;
  int vectorsWereFixed = 0;

  // vtkDebugWithObjectMacro(nullptr, "Vectors for block: ");
  // vtkDebugWithObjectMacro(nullptr, "  X: " << this->XYZArrays[0]->GetNumberOfTuples());
  // vtkDebugWithObjectMacro(nullptr, "  Y: " << this->XYZArrays[1]->GetNumberOfTuples());
  // vtkDebugWithObjectMacro(nullptr, "  Z: " << this->XYZArrays[2]->GetNumberOfTuples());
  // vtkDebugWithObjectMacro(nullptr, " Dims: " << coutVector3(this->Dimensions));
  // vtkDebugWithObjectMacro(nullptr, " Bool: " << this->IsFixed());

  double minV, maxV;
  for (i = 0, j = 0; i < 3; i++, j++)
  {
    if (this->Dimensions[i] == 1)
    {
      realExtents[j++] = 0;
      realExtents[j] = 1;
      realDims[i] = 1;
      ca[i] = nullptr;
      continue;
    }

    minV = MinBlockBound(i);
    maxV = MaxBlockBound(i);
    // vtkDebugWithObjectMacro(
    //  nullptr, "Bounds[" << (j) << "] = " << minV << " Bounds[" << (j + 1) << "] = " << maxV);
    ca[i] = this->XYZArrays[i];
    if (minV < minP[i])
    {
      realExtents[j] = 1;
      --extents[j + 1];
      hasBadGhostCells = 1;
      if (!this->IsFixed())
      {
        this->XYZArrays[i]->RemoveFirstTuple();
        vectorsWereFixed = 1;
      }
    }
    else
    {
      realExtents[j] = 0;
    }
    j++;

    if (maxV > maxP[i])
    {
      realExtents[j] = this->Dimensions[i] - 1;
      --extents[j];
      hasBadGhostCells = 1;
      if (!this->IsFixed())
      {
        this->XYZArrays[i]->RemoveLastTuple();
        vectorsWereFixed = 1;
      }
    }
    else
    {
      realExtents[j] = this->Dimensions[i];
    }
    realDims[i] = realExtents[j] - realExtents[j - 1];
  }
  if (vectorsWereFixed)
  {
    this->SavedExtents[0] = extents[0];
    this->SavedExtents[1] = extents[1];
    this->SavedExtents[2] = extents[2];
    this->SavedExtents[3] = extents[3];
    this->SavedExtents[4] = extents[4];
    this->SavedExtents[5] = extents[5];
    this->SavedRealExtents[0] = realExtents[0];
    this->SavedRealExtents[1] = realExtents[1];
    this->SavedRealExtents[2] = realExtents[2];
    this->SavedRealExtents[3] = realExtents[3];
    this->SavedRealExtents[4] = realExtents[4];
    this->SavedRealExtents[5] = realExtents[5];
    this->SavedRealDims[0] = realDims[0];
    this->SavedRealDims[1] = realDims[1];
    this->SavedRealDims[2] = realDims[2];
    this->Status.Fixed = 1;
  }
  return hasBadGhostCells;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotBlock::Read(int isAMR, int fileVersion, vtkSpyPlotIStream* stream)
{
  if (isAMR)
  {
    this->Status.AMR = 1;
  }
  else
  {
    this->Status.AMR = 0;
  }

  if (fileVersion >= 103)
  {
    // Lets make the read in one call
    // Here are the original reads
    //
    // Read in the dimensions of the block (Read 3 ints)
    // Read in the allocation state of the block (Read 1 int)
    // Read in the active state of the block (Read 1 int)
    // Read in the level of the block (Read 1 int)
    // read in bounds, but don't do anything with them (Read 6 ints)

    int temp[12];

    if (!stream->ReadInt32s(temp, 12))
    {
      vtkGenericWarningMacro("Could not read in block");
      return 0;
    }

    this->Dimensions[0] = temp[0];
    this->Dimensions[1] = temp[1];
    this->Dimensions[2] = temp[2];

    if (temp[3])
      this->Status.Allocated = 1;
    else
      this->Status.Allocated = 0;

    if (temp[4])
      this->Status.Active = 1;
    else
      this->Status.Active = 0;

    this->Level = temp[5];
  }
  else
  {
    // Lets make the read in one call
    // Here are the original reads
    //
    // Read in the dimensions of the block (Read 3 ints)
    // Read in the allocation state of the block (Read 1 int)
    // Read in the active state of the block (Read 1 int)
    // Read in the level of the block (Read 1 int)

    int temp[6];

    if (!stream->ReadInt32s(temp, 6))
    {
      vtkGenericWarningMacro("Could not read in block");
      return 0;
    }

    this->Dimensions[0] = temp[0];
    this->Dimensions[1] = temp[1];
    this->Dimensions[2] = temp[2];

    if (temp[3])
      this->Status.Allocated = 1;
    else
      this->Status.Allocated = 0;

    if (temp[4])
      this->Status.Active = 1;
    else
      this->Status.Active = 0;

    this->Level = temp[5];
  }

  int i;
  if (this->IsAllocated())
  {
    for (i = 0; i < 3; i++)
    {
      if (!this->XYZArrays[i])
      {
        this->XYZArrays[i] = vtkFloatArray::New();
      }
      this->XYZArrays[i]->SetNumberOfTuples(this->Dimensions[i] + 1);
    }
  }
  else
  {
    for (i = 0; i < 3; i++)
    {
      if (this->XYZArrays[i])
      {
        this->XYZArrays[i]->Delete();
        this->XYZArrays[i] = nullptr;
      }
    }
  }
  this->Status.Fixed = 0;
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSpyPlotBlock::Scan(vtkSpyPlotIStream* stream, unsigned char* isAllocated, int fileVersion)
{
  int temp[12];

  //
  // This function's whole purpose in life is to move the read location
  // past a block header, and return whether the allocated block
  // flag is set.
  //
  // Lets make the read in one call.
  // Here are the original reads:
  //
  // Read in the dimensions of the block (Read 3 words)
  // Read in the allocation state of the block (Read 1 word)
  // Read in the active state of the block (Read 1 word)
  // Read in the level of the block (Read 1 word)
  if (fileVersion >= 103)
  {
    if (!stream->ReadInt32sNoSwap(temp, 12))
    {
      vtkGenericWarningMacro("Could not read in block");
      return 0;
    }
  }
  else
  {
    if (!stream->ReadInt32sNoSwap(temp, 6))
    {
      vtkGenericWarningMacro("Could not read in block");
      return 0;
    }
  }

  //
  // Dirty trick here.
  //
  // Data coming from ReadInt32sNoSwap is in reverse byte order.
  //   However, all we care about is whether it is a 0 or non 0.
  //   Doesn't matter where the bytes or bits are.  Thus, test
  //   the int unswapped.
  //
  //   Faster yet would be moving bits, but probably not necessary.
  //
  if (temp[3])
    *isAllocated = 1;
  else
    *isAllocated = 0;

  return 1;
}

//-----------------------------------------------------------------------------
// Scan, but for 16 blocks at a time.  Performance code.
//
int vtkSpyPlotBlock::Scan16(vtkSpyPlotIStream* stream, unsigned char* isAllocated, int fileVersion)
{
  int temp[16 * 12];
  int i;

  //
  // This function's whole purpose in life is to move the read location
  //   past a block header, and return whether the allocated block
  //   flag is set.
  //
  //   Note that we are processing 16 block headers at a time.
  //
  //   Lets make the read in one call.
  //   Here was the originals:
  // Read in the dimensions of the block (Read 3 words)
  // Read in the allocation state of the block (Read 1 word)
  // Read in the active state of the block (Read 1 word)
  // Read in the level of the block (Read 1 word)
  // if (fileVersion >= 103)
  //   read in bounds, but don't do anything with them (Read 6 word)

  if (fileVersion >= 103)
  {
    if (!stream->ReadInt32sNoSwap(temp, 16 * 12))
    {
      vtkGenericWarningMacro("Could not read in block");
      return 0;
    }

    // Dirty trick here. See vtkSpyPlotBlock::Scan() for details
    for (i = 0; i < 16; i++)
    {
      if (*(temp + 12 * i + 3))
        *(isAllocated + i) = 1;
      else
        *(isAllocated + i) = 0;
    }
  }
  else
  {
    if (!stream->ReadInt32sNoSwap(temp, 16 * 6))
    {
      vtkGenericWarningMacro("Could not read in block");
      return 0;
    }

    // Dirty trick here. See vtkSpyPlotBlock::Scan() for details
    for (i = 0; i < 16; i++)
    {
      if (*(temp + 6 * i + 3))
        *(isAllocated + i) = 1;
      else
        *(isAllocated + i) = 0;
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::SetCoordinateSystem(const int& coordinateSystem)
{
  // if the number inputed is invalid we will make it a 3D coordinate
  switch (coordinateSystem)
  {
    case vtkSpyPlotBlock::Cylinder1D:
      this->CoordSystem = vtkSpyPlotBlock::Cylinder1D;
      break;
    case vtkSpyPlotBlock::Sphere1D:
      this->CoordSystem = vtkSpyPlotBlock::Sphere1D;
      break;
    case vtkSpyPlotBlock::Cartesian2D:
      this->CoordSystem = vtkSpyPlotBlock::Cartesian2D;
      break;
    case vtkSpyPlotBlock::Cylinder2D:
      this->CoordSystem = vtkSpyPlotBlock::Cylinder2D;
      break;
    case vtkSpyPlotBlock::Cartesian3D:
    default: // if unknown make it 3D
      this->CoordSystem = vtkSpyPlotBlock::Cartesian3D;
      break;
  }
}

//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::ComputeDerivedVariables(vtkCellData* data, const int& numberOfMaterials,
  vtkDataArray** materialMasses, vtkDataArray** materialVolumeFractions,
  const int& downConvertVolumeFraction) const
{
  // this->SavedRealDims are to be used when this->Status.Fixed is true,
  // otherwise this->SavedRealDims is never set and we must use
  // this->Dimensions.
  const int* dimensions = this->Status.Fixed ? this->SavedRealDims : this->Dimensions;
  vtkIdType arraySize = dimensions[0] * dimensions[1] * dimensions[2];

  // Make sure we convert zero value dims to 1.
  if (arraySize <= 0)
  {
    return;
  }

  typedef std::map<int, vtkDoubleArray*> MapOfArrays;
  MapOfArrays materialDensityArrays;
  for (int i = 0; i < numberOfMaterials; i++)
  {
    // only create density arrays for materials that we have all the needed information for
    bool validDensity = materialMasses[i] != nullptr && materialVolumeFractions[i] != nullptr;
    if (validDensity)
    {
      vtkDoubleArray* materialDensity = vtkDoubleArray::New();
      std::stringstream buffer;
      buffer << "Derived Density - " << i + 1;
      materialDensity->SetName(buffer.str().c_str());
      materialDensity->SetNumberOfComponents(1);
      materialDensity->SetNumberOfTuples(arraySize);

      materialDensityArrays[i] = materialDensity;
      data->AddArray(materialDensity);
      materialDensity->FastDelete();
    }
  }

  if (materialDensityArrays.size() == 0)
  {
    // we can't compute derived variables as we have no materials
    return;
  }

  vtkDoubleArray* volumeArray = vtkDoubleArray::New();
  volumeArray->SetName("Derived Volume");
  volumeArray->SetNumberOfValues(arraySize);

  vtkDoubleArray* averageDensityArray = vtkDoubleArray::New();
  averageDensityArray->SetName("Derived Average Density");
  averageDensityArray->SetNumberOfValues(arraySize);

  data->AddArray(volumeArray);
  volumeArray->FastDelete();

  data->AddArray(averageDensityArray);
  averageDensityArray->FastDelete();

  vtkIdType pos = 0;
  for (int k = 0; k < dimensions[2]; k++)
  {
    for (int j = 0; j < dimensions[1]; j++)
    {
      for (int i = 0; i < dimensions[0]; i++, pos++)
      {
        // sum the mass for each each material
        volumeArray->SetValue(pos, this->GetCellVolume(i, j, k));

        double mass_sum = 0, occupied_volume_sum = 0;
        for (MapOfArrays::iterator iter = materialDensityArrays.begin();
             iter != materialDensityArrays.end(); ++iter)
        {
          int index = iter->first;
          vtkDoubleArray* materialDensity = iter->second;
          double material_mass = 0, material_volume = 0;
          if (downConvertVolumeFraction)
          {
            vtkUnsignedCharArray* materialFraction =
              static_cast<vtkUnsignedCharArray*>(materialVolumeFractions[index]);
            this->ComputeMaterialDensity(pos, materialMasses[index], materialFraction, volumeArray,
              materialDensity, &material_mass, &material_volume);
          }
          else
          {
            vtkFloatArray* materialFraction =
              static_cast<vtkFloatArray*>(materialVolumeFractions[index]);
            this->ComputeMaterialDensity(pos, materialMasses[index], materialFraction, volumeArray,
              materialDensity, &material_mass, &material_volume);
          }
          mass_sum += material_mass;
          occupied_volume_sum += material_volume;
        }

        double average_density = (occupied_volume_sum == 0) ? 0 : (mass_sum / occupied_volume_sum);
        averageDensityArray->SetValue(pos, average_density);
      }
    }
  }
}

//-----------------------------------------------------------------------------
double vtkSpyPlotBlock::GetCellVolume(int i, int j, int k) const
{
  // this->SavedRealDims are to be used when this->Status.Fixed is true,
  // otherwise this->SavedRealDims is never set and we must use
  // this->Dimensions.
  const int* dimensions = this->Status.Fixed ? this->SavedRealDims : this->Dimensions;

  // make sure the cell index is valid;
  double volume = -1;
  if (i < 0 || i >= dimensions[0] || j < 0 || j >= dimensions[1] || k < 0 || k >= dimensions[2])
  {
    // invalid coordinate
    return volume;
  }

  // determine the volume by the blocks coordinate system:
  float* x = static_cast<float*>(this->XYZArrays[0]->GetVoidPointer(0));
  float* y = static_cast<float*>(this->XYZArrays[1]->GetVoidPointer(0));
  float* z = static_cast<float*>(this->XYZArrays[2]->GetVoidPointer(0));
  switch (this->CoordSystem)
  {
    case vtkSpyPlotBlock::Cylinder1D:
      volume = vtkMath::Pi() * (x[i + 1] * x[i + 1] - x[i] * x[i]);
      ;
      break;
    case vtkSpyPlotBlock::Sphere1D:
      volume = 4 * (vtkMath::Pi() / 3) * (x[i + 1] * x[i + 1] * x[i + 1] - x[i] * x[i] * x[i]);
      break;
    case vtkSpyPlotBlock::Cartesian2D:
      volume = (y[j + 1] - y[j]) * (x[i + 1] - x[i]);
      break;
    case vtkSpyPlotBlock::Cylinder2D:
      volume = vtkMath::Pi() * (y[j + 1] - y[j]) * (x[i + 1] * x[i + 1] - x[i] * x[i]);
      break;
    case vtkSpyPlotBlock::Cartesian3D:
      volume = (z[k + 1] - z[k]) * (y[j + 1] - y[j]) * (x[i + 1] - x[i]);
      break;
  }
  return volume;
}

//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::ComputeMaterialDensity(vtkIdType pos, vtkDataArray* materialMass,
  vtkUnsignedCharArray* materialFraction, vtkDoubleArray* volumes, vtkDoubleArray* materialDensity,
  double* material_mass, double* material_volume) const
{
  double mass = -1, volume = -1, density = -1, volfrac = -1;

  mass = materialMass->GetTuple1(pos);
  volfrac = static_cast<int>(materialFraction->GetValue(pos)) / 255.0;
  volume = volumes->GetValue(pos);
  if (volfrac == 0 || mass == 0 || volume == 0)
  {
    density = 0;
    *material_mass = 0;
    *material_volume = 0;
  }
  else
  {
    density = mass / (volume * volfrac);

    *material_mass = mass;
    *material_volume = volume * volfrac;
  }

  materialDensity->SetValue(pos, density);
}

//-----------------------------------------------------------------------------
void vtkSpyPlotBlock::ComputeMaterialDensity(vtkIdType pos, vtkDataArray* materialMass,
  vtkFloatArray* materialFraction, vtkDoubleArray* volumes, vtkDoubleArray* materialDensity,
  double* material_mass, double* material_volume) const
{
  double mass = -1, volume = -1, density = -1, volfrac = -1;

  mass = materialMass->GetTuple1(pos);
  volfrac = materialFraction->GetValue(pos);
  volume = volumes->GetValue(pos);

  if (volfrac == 0 || mass == 0 || volume == 0)
  {
    density = 0;

    *material_mass = 0;
    *material_volume = 0;
  }
  else
  {
    density = mass / (volume * volfrac);

    *material_mass = mass;
    *material_volume = volume * volfrac;
  }
  materialDensity->SetValue(pos, density);
}

//-----------------------------------------------------------------------------
/* SetGeometry - sets the geometric definition of the block's ith direction
   encodeInfo is a runlength delta encoded string of size infoSize

   DeltaRunLengthEncoded Description:
   run-length-encodes the data pointed to by *data, placing
   the result in *out. n is the number of doubles to encode. n_out
   is the number of bytes used for the compression (and stored at
   *out). delta is the smallest change of adjacent values that will
   be accepted (changes smaller than delta will be ignored).

*/

int vtkSpyPlotBlock::SetGeometry(int dir, const unsigned char* encodedInfo, int infoSize)
{
  int compIndex = 0, inIndex = 0;
  int compSize = this->Dimensions[dir] + 1;
  const unsigned char* ptmp = encodedInfo;

  /* Run-length decode */

  // Get first value
  float val;
  memcpy(&val, ptmp, sizeof(float));
  vtkByteSwap::SwapBE(&val);
  ptmp += 4;

  // Get delta
  float delta;
  memcpy(&delta, ptmp, sizeof(float));
  vtkByteSwap::SwapBE(&delta);
  ptmp += 4;

  if (!this->XYZArrays[dir])
  {
    vtkErrorWithObjectMacro(nullptr, "Coordinate Array has not been allocated");
    return 0;
  }

  float* comp = this->XYZArrays[dir]->GetPointer(0);

  // Now loop around until I get to the end of
  // the input array
  inIndex += 8;
  while ((compIndex < compSize) && (inIndex < infoSize))
  {
    // Okay get the run length
    unsigned char runLength = *ptmp;
    ptmp++;
    if (runLength < 128)
    {
      ptmp += 4;
      // Now populate the comp data
      int k;
      for (k = 0; k < runLength; ++k)
      {
        if (compIndex >= compSize)
        {
          vtkErrorWithObjectMacro(nullptr, "Problem doing RLD decode. "
              << "Too much data generated. Expected: " << compSize);
          return 0;
        }
        comp[compIndex] = val + compIndex * delta;
        compIndex++;
      }
      inIndex += 5;
    }
    else // runLength >= 128
    {
      int k;
      for (k = 0; k < runLength - 128; ++k)
      {
        if (compIndex >= compSize)
        {
          vtkErrorWithObjectMacro(nullptr, "Problem doing RLD decode. "
              << "Too much data generated. Expected: " << compSize);
          return 0;
        }
        float nval;
        memcpy(&nval, ptmp, sizeof(float));
        vtkByteSwap::SwapBE(&nval);
        comp[compIndex] = nval + compIndex * delta;
        compIndex++;
        ptmp += 4;
      }
      inIndex += 4 * (runLength - 128) + 1;
    }
  } // while

  return 1;
}
