/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTilesHelper.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkTilesHelper);
//----------------------------------------------------------------------------
vtkTilesHelper::vtkTilesHelper()
{
  this->TileDimensions[0] = 1;
  this->TileDimensions[1] = 1;
  this->TileMullions[0] = this->TileMullions[1] = 0;
  this->TileWindowSize[0] = this->TileWindowSize[1] = 300;
}

//----------------------------------------------------------------------------
vtkTilesHelper::~vtkTilesHelper()
{
}

//----------------------------------------------------------------------------
static double vtkMin(double x, double y)
{
  return x < y ? x : y;
}

//----------------------------------------------------------------------------
static double vtkMax(double x, double y)
{
  return x > y ? x : y;
}

//----------------------------------------------------------------------------
void vtkTilesHelper::GetTileIndex(int rank, int* tileX, int* tileY)
{
  int x = rank % this->TileDimensions[0];
  int y = rank / this->TileDimensions[0];
  if (y >= this->TileDimensions[1])
  {
    y = this->TileDimensions[1] - 1;
  }

  // invert y so that the 0th rank corresponds to the top-left tile rather than
  // bottom left tile.
  y = this->TileDimensions[1] - y - 1;
  *tileX = x;
  *tileY = y;
}

//----------------------------------------------------------------------------
bool vtkTilesHelper::GetNormalizedTileViewport(
  const double* viewport, int rank, double out_tile_viewport[4])
{
  double normalized_mullions[2];
  normalized_mullions[0] = static_cast<double>(this->TileMullions[0]) /
    (this->TileWindowSize[0] * this->TileDimensions[0]);
  normalized_mullions[1] = static_cast<double>(this->TileMullions[1]) /
    (this->TileWindowSize[1] * this->TileDimensions[1]);

  // The size of the tile as a fraction of the total display size.
  double normalized_tile_size[2];
  normalized_tile_size[0] = 1.0 / this->TileDimensions[0];
  normalized_tile_size[1] = 1.0 / this->TileDimensions[1];

  int x, y;
  this->GetTileIndex(rank, &x, &y);

  out_tile_viewport[0] = x * normalized_tile_size[0];
  out_tile_viewport[1] = y * normalized_tile_size[1];
  out_tile_viewport[2] = out_tile_viewport[0] + normalized_tile_size[0];
  out_tile_viewport[3] = out_tile_viewport[1] + normalized_tile_size[1];

  // Now the tile for the given rank is showing the normalized viewport
  // indicated by out_tile_viewport. Now, we intersect it with the
  // viewport to return where the current viewport maps in the tile.
  if (viewport)
  {
    out_tile_viewport[0] = ::vtkMax(viewport[0], out_tile_viewport[0]);
    out_tile_viewport[1] = ::vtkMax(viewport[1], out_tile_viewport[1]);
    out_tile_viewport[2] = ::vtkMin(viewport[2], out_tile_viewport[2]);
    out_tile_viewport[3] = ::vtkMin(viewport[3], out_tile_viewport[3]);
  }

  if (out_tile_viewport[2] <= out_tile_viewport[0] || out_tile_viewport[3] <= out_tile_viewport[1])
  {
    return false;
  }

  // Shift the entire viewport around using the mullions.
  out_tile_viewport[0] += x * normalized_mullions[0];
  out_tile_viewport[1] += y * normalized_mullions[1];
  out_tile_viewport[2] += x * normalized_mullions[0];
  out_tile_viewport[3] += y * normalized_mullions[1];
  return true;
}

//----------------------------------------------------------------------------
bool vtkTilesHelper::GetTileViewport(const double* viewport, int rank, int out_tile_viewport[4])
{
  double normalized_tile_viewport[4];
  if (this->GetNormalizedTileViewport(viewport, rank, normalized_tile_viewport))
  {
    out_tile_viewport[0] = static_cast<int>(
      normalized_tile_viewport[0] * this->TileWindowSize[0] * this->TileDimensions[0] + 0.5);
    out_tile_viewport[1] = static_cast<int>(
      normalized_tile_viewport[1] * this->TileWindowSize[1] * this->TileDimensions[1] + 0.5);
    out_tile_viewport[2] =
      static_cast<int>(
        normalized_tile_viewport[2] * this->TileWindowSize[0] * this->TileDimensions[0] + 0.5) -
      1;
    out_tile_viewport[3] =
      static_cast<int>(
        normalized_tile_viewport[3] * this->TileWindowSize[1] * this->TileDimensions[1] + 0.5) -
      1;
    // due to rounding, we although normalized ranges maybe valid, we may end
    // up with invalid integral ranges. So test again.
    if (out_tile_viewport[2] <= out_tile_viewport[0] ||
      out_tile_viewport[3] <= out_tile_viewport[1])
    {
      return false;
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkTilesHelper::GetPhysicalViewport(
  const double* global_viewport, int rank, double out_phyiscal_viewport[4])
{
  // Get the normalized tile-viewport for the full tile on this rank.
  double full_tile_viewport[4];
  this->GetNormalizedTileViewport(NULL, rank, full_tile_viewport);

  // effectively, clamp the  global_viewport to the full_tile_viewport.
  double clamped_global_viewport[4];
  if (this->GetNormalizedTileViewport(global_viewport, rank, clamped_global_viewport) == false)
  {
    return false;
  }

  double dx = full_tile_viewport[2] - full_tile_viewport[0];
  double dy = full_tile_viewport[3] - full_tile_viewport[1];

  out_phyiscal_viewport[0] = (clamped_global_viewport[0] - full_tile_viewport[0]) / dx;
  out_phyiscal_viewport[1] = (clamped_global_viewport[1] - full_tile_viewport[1]) / dy;
  out_phyiscal_viewport[2] = (clamped_global_viewport[2] - full_tile_viewport[0]) / dx;
  out_phyiscal_viewport[3] = (clamped_global_viewport[3] - full_tile_viewport[1]) / dy;
  return true;
}

//----------------------------------------------------------------------------
void vtkTilesHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
