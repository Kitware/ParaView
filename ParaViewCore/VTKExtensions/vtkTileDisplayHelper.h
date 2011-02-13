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
// .NAME vtkTileDisplayHelper - used on server side in tile display mode, to
// ensure that the tiles from multiple views are rendered correctly.
// .SECTION Description
// vtkTileDisplayHelper is used on server side in tile display mode, to
// ensure that the tiles from multiple views are rendered correctly. This is
// required since in multi-view configurations, only 1 view is rendered at a
// time, and when a view is being rendered, it may affect the renderings from
// other view which it must restore for the tile display to show the results
// correctly. We use this helper to keep buffered images from all views so that
// they can be restored.

#ifndef __vtkTileDisplayHelper_h
#define __vtkTileDisplayHelper_h

#include "vtkObject.h"
#include "vtkSynchronizedRenderers.h" // needed for vtkRawImage.

class vtkRenderer;

class VTK_EXPORT vtkTileDisplayHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkTileDisplayHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the singleton.
  static vtkTileDisplayHelper* GetInstance();

  // Description:
  // Register a tile.
  void SetTile(void* key,
    double viewport[4], vtkRenderer* renderer,
    vtkSynchronizedRenderers::vtkRawImage& tile);

  // Description:
  // Erase a tile.
  void EraseTile(void* key);

  // Description:
  // Flush the tiles.
  void FlushTiles(void* key);

//BTX
protected:
  vtkTileDisplayHelper();
  ~vtkTileDisplayHelper();
  static vtkTileDisplayHelper* New();

private:
  vtkTileDisplayHelper(const vtkTileDisplayHelper&); // Not implemented
  void operator=(const vtkTileDisplayHelper&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
