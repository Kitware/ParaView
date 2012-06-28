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
// .NAME vtkTilesHelper - this is a helper class that handles viewport
// computations when rendering for tile-displays.
// This assumes that all tiles have the same pixel-size.
// .SECTION Description
//

#ifndef __vtkTilesHelper_h
#define __vtkTilesHelper_h

#include "vtkObject.h"

class VTK_EXPORT vtkTilesHelper : public vtkObject
{
public:
  static vtkTilesHelper* New();
  vtkTypeMacro(vtkTilesHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the tile dimensions. Default is (1, 1).
  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);

  // Description:
  // Set the tile mullions in pixels. Use negative numbers to indicate overlap
  // between tiles.
  vtkSetVector2Macro(TileMullions, int);
  vtkGetVector2Macro(TileMullions, int);

  // Description:
  // Set the tile size i.e. the size of the render window for a tile. We assumes
  // that all tiles have the same size (since that's a requirement for IceT).
  vtkSetVector2Macro(TileWindowSize, int);
  vtkGetVector2Macro(TileWindowSize, int);

  // Description:
  // Returns (x-origin, y-origin, x-max, y-max) in pixels for a tile with given
  // \c rank. \c viewport is in normalized display coordinates i.e. in range
  // [0, 1] indicating the viewport covered by the current renderer on the whole
  // i.e. treating all tiles as one large display if TileDimensions > (1, 1).
  // Returns false to indicate the result hasn't been computed.
  bool GetTileViewport(const double *viewport, int rank,
    int out_tile_viewport[4]);

  // Description:
  // Same as GetTileViewport() except that the returns values are in
  // normalized-display coordinates instead of display coordinates.
  // Returns false to indicate the result hasn't been computed.
  bool GetNormalizedTileViewport(const double* viewport, int rank,
    double out_tile_viewport[4]);

  // Description:
  // Given a global-viewport for a renderer, returns the physical viewport on
  // the rank indicated.
  // Returns false to indicate the result hasn't been computed.
  bool GetPhysicalViewport(const double *global_viewport, int rank,
    double out_phyiscal_viewport[4]);

  // Description:
  // Given the rank, returns the tile location.
  void GetTileIndex(int rank, int* tileX, int* tileY);

//BTX
protected:
  vtkTilesHelper();
  ~vtkTilesHelper();

  int TileDimensions[2];
  int TileMullions[2];
  int TileWindowSize[2];

private:
  vtkTilesHelper(const vtkTilesHelper&); // Not implemented
  void operator=(const vtkTilesHelper&); // Not implemented
//ETX
};

#endif
