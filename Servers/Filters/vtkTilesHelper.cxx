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
vtkCxxRevisionMacro(vtkTilesHelper, "$Revision$");
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
const double* vtkTilesHelper::GetNormalizedTileViewport(double* viewport, int rank)
{
  double normalized_mullions[2];
  normalized_mullions[0] = static_cast<double>(this->TileMullions[0])/
    (this->TileWindowSize[0] * this->TileDimensions[0]);
  normalized_mullions[1] = static_cast<double>(this->TileMullions[1])/
    (this->TileWindowSize[1] * this->TileDimensions[1]);

  // The size of the tile as a fraction of the total display size.
  double normalized_tile_size[2];
  normalized_tile_size[0] = 1.0/this->TileDimensions[0];
  normalized_tile_size[1] = 1.0/this->TileDimensions[1];

  // The spacing of the tiles (including the mullions).
  double normalized_tile_spacing[2];
  normalized_tile_spacing[0] = normalized_tile_size[0] + normalized_mullions[0];
  normalized_tile_spacing[1] = normalized_tile_size[1] + normalized_mullions[1];

  int x = rank % this->TileDimensions[0];
  int y = rank / this->TileDimensions[0];
  if (y >= this->TileDimensions[1])
    {
    y = this->TileDimensions[1] - 1;
    }

  // invert y so that the 0th rank corresponds to the top-left tile rather than
  // bottom left tile.
  y = this->TileDimensions[1] - y - 1;

  static double normalized_tile_viewport[4];
  normalized_tile_viewport[0] = x * normalized_tile_spacing[0];
  normalized_tile_viewport[1] = y * normalized_tile_spacing[1];
  normalized_tile_viewport[2] = normalized_tile_viewport[0] + normalized_tile_size[0];
  normalized_tile_viewport[3] = normalized_tile_viewport[1] + normalized_tile_size[1];

  // Now the tile for the given rank is showing the normalized viewport
  // indicated by normalized_tile_viewport. Now, we intersect it with the
  // viewport to return where the current viewport maps in the tile.
  if (viewport)
    {
    normalized_tile_viewport[0] = ::vtkMax(viewport[0],
      normalized_tile_viewport[0]);
    normalized_tile_viewport[1] = ::vtkMax(viewport[1],
      normalized_tile_viewport[1]);
    normalized_tile_viewport[2] = ::vtkMin(viewport[2],
      normalized_tile_viewport[2]);
    normalized_tile_viewport[3] = ::vtkMin(viewport[3],
      normalized_tile_viewport[3]);
    }

  if (normalized_tile_viewport[2] < normalized_tile_viewport[0] ||
    normalized_tile_viewport[3] < normalized_tile_viewport[1])
    {
    return NULL;
    }

  return normalized_tile_viewport;
}

//----------------------------------------------------------------------------
const int* vtkTilesHelper::GetTileViewport(double *viewport, int rank)
{
  const double* normalized_tile_viewport = this->GetNormalizedTileViewport(viewport, rank);
  if (normalized_tile_viewport)
    {
    static int tile_viewport[4];
    tile_viewport[0] = normalized_tile_viewport[0] * this->TileWindowSize[0] *
      this->TileDimensions[0];
    tile_viewport[1] = normalized_tile_viewport[1] * this->TileWindowSize[1] *
      this->TileDimensions[1];
    tile_viewport[2] = 0.5 + normalized_tile_viewport[2] * this->TileWindowSize[0] *
      this->TileDimensions[0];
    tile_viewport[3] = 0.5 + normalized_tile_viewport[3] * this->TileWindowSize[1] *
      this->TileDimensions[1];
    return tile_viewport;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkTilesHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
