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
/**
 * @class   vtkTilesHelper
 * @brief   this is a helper class that handles viewport
 * computations when rendering for tile-displays.
 * This assumes that all tiles have the same pixel-size.
 *
 *
*/

#ifndef vtkTilesHelper_h
#define vtkTilesHelper_h

#include "vtkObject.h"
#include "vtkRemotingViewsModule.h" // needed for export macro
#include "vtkVector.h"              // for vtkVector

class VTKREMOTINGVIEWS_EXPORT vtkTilesHelper : public vtkObject
{
public:
  static vtkTilesHelper* New();
  vtkTypeMacro(vtkTilesHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the tile dimensions. Default is (1, 1).
   */
  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);
  //@}

  //@{
  /**
   * Set the tile mullions in pixels. Use negative numbers to indicate overlap
   * between tiles.
   */
  vtkSetVector2Macro(TileMullions, int);
  vtkGetVector2Macro(TileMullions, int);
  //@}

  //@{
  /**
   * Set the tile size i.e. the size of the render window for a single tile. An assumption,
   * in ParaView is that all tiles will be of the same size.
   */
  vtkSetVector2Macro(TileWindowSize, int);
  vtkGetVector2Macro(TileWindowSize, int);
  //@}

  /**
   * For the specified `rank`, returns the tile size and origin of the tile
   * rendered by the rank in display coordinates. If the rank is not expected to
   * render an tile then returns false and `size` and `lowerLeft` will be left
   * unchanged. Otherwise, returns true after updating `size` and `lowerLeft`
   * appropriately.
   */
  bool GetTiledSizeAndOrigin(int rank, vtkVector2i& size, vtkVector2i& lowerLeft) const;

  /**
   * A `GetTiledSizeAndOrigin` overload that takes in a `viewport` expressed as
   * `(xmin, ymin, xmax, ymax)` where each coordinate is in the range `[0, 1.0]`
   * (same as vtkViewport::SetViewport). The size and origin returned are
   * limited to the specified viewport.
   */
  bool GetTiledSizeAndOrigin(
    int rank, vtkVector2i& size, vtkVector2i& lowerLeft, vtkVector4d viewport) const;

  /**
   * Provides the viewport for the tile displayed on the rank, if any. Returns
   * false if the rank is not expected to display a tile. Otherwise returns true
   * after updating `tile_viewport` to the result.
   */
  bool GetTileViewport(int rank, vtkVector4d& tile_viewport) const;

  /**
   * Given the rank, returns the tile location. Returns false if the rank is not
   * expected to render any tile.
   */
  bool GetTileIndex(int rank, int* tileX, int* tileY) const;

  /**
   * Returns true if current rank will render a tile.
   */
  bool GetTileEnabled(int rank) const
  {
    int x, y;
    return this->GetTileIndex(rank, &x, &y);
  }

protected:
  vtkTilesHelper();
  ~vtkTilesHelper() override;

  int TileDimensions[2];
  int TileMullions[2];
  int TileWindowSize[2];

private:
  vtkTilesHelper(const vtkTilesHelper&) = delete;
  void operator=(const vtkTilesHelper&) = delete;
};

#endif
