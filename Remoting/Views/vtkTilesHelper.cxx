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
#include "vtkVectorOperators.h"

#include <algorithm>
#include <cassert>

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
vtkTilesHelper::~vtkTilesHelper() = default;

//----------------------------------------------------------------------------
bool vtkTilesHelper::GetTileIndex(int rank, int* tileX, int* tileY) const
{
  if (rank < (this->TileDimensions[0] * this->TileDimensions[1]))
  {
    int x = rank % this->TileDimensions[0];
    int y = rank / this->TileDimensions[0];
    assert(y < this->TileDimensions[1]);

    // invert y so that the 0th rank corresponds to the top-left tile rather than
    // bottom left tile.
    y = (this->TileDimensions[1] - 1) - y;
    *tileX = x;
    *tileY = y;
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkTilesHelper::GetTileViewport(int rank, vtkVector4d& tile_viewport) const
{
  vtkVector2i size, origin;
  if (!this->GetTiledSizeAndOrigin(rank, size, origin))
  {
    return false;
  }

  const vtkVector2i full_size(
    this->TileDimensions[0] * (this->TileMullions[0] + this->TileWindowSize[0]) -
      this->TileMullions[0],
    this->TileDimensions[1] * (this->TileMullions[1] + this->TileWindowSize[1]) -
      this->TileMullions[1]);

  tile_viewport[0] = static_cast<double>(origin[0]) / full_size[0];
  tile_viewport[1] = static_cast<double>(origin[1]) / full_size[1];
  tile_viewport[2] = static_cast<double>(origin[0] + size[0]) / full_size[0];
  tile_viewport[3] = static_cast<double>(origin[1] + size[1]) / full_size[1];
  return true;
}

//----------------------------------------------------------------------------
bool vtkTilesHelper::GetTiledSizeAndOrigin(
  int rank, vtkVector2i& size, vtkVector2i& lowerLeft) const
{
  vtkVector2i tindex;
  if (!this->GetTileIndex(rank, &tindex[0], &tindex[1]))
  {
    return false;
  }

  size[0] = this->TileWindowSize[0];
  size[1] = this->TileWindowSize[1];
  lowerLeft[0] = tindex[0] * (this->TileWindowSize[0] + this->TileMullions[0]);
  lowerLeft[1] = tindex[1] * (this->TileWindowSize[1] + this->TileMullions[1]);
  return true;
}

//----------------------------------------------------------------------------
bool vtkTilesHelper::GetTiledSizeAndOrigin(
  int rank, vtkVector2i& size, vtkVector2i& lowerLeft, vtkVector4d viewport) const
{
  const vtkVector2i full_size(
    this->TileDimensions[0] * (this->TileMullions[0] + this->TileWindowSize[0]) -
      this->TileMullions[0],
    this->TileDimensions[1] * (this->TileMullions[1] + this->TileWindowSize[1]) -
      this->TileMullions[1]);

  vtkVector2i vpLowerLeft(static_cast<int>(viewport[0] * full_size[0] + 0.5),
    static_cast<int>(viewport[1] * full_size[1] + 0.5));
  vtkVector2i vpUpperRight(static_cast<int>(viewport[2] * full_size[0] + 0.5),
    static_cast<int>(viewport[3] * full_size[1] + 0.5));

  if (!this->GetTiledSizeAndOrigin(rank, size, lowerLeft))
  {
    return false;
  }
  const vtkVector2i tUpperRight = lowerLeft + size;

  vpLowerLeft[0] = std::max(vpLowerLeft[0], lowerLeft[0]);
  vpLowerLeft[1] = std::max(vpLowerLeft[1], lowerLeft[1]);

  vpUpperRight[0] = std::min(vpUpperRight[0], tUpperRight[0]);
  vpUpperRight[1] = std::min(vpUpperRight[1], tUpperRight[1]);

  lowerLeft = vpLowerLeft;
  size = vpUpperRight - vpLowerLeft;
  size[0] = std::max(0, size[0]);
  size[1] = std::max(0, size[1]);
  return (size[0] > 0 && size[1] > 0);
}

//----------------------------------------------------------------------------
void vtkTilesHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
