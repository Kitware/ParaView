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
#include "vtkTileDisplayHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

#include <vtkstd/map>

class vtkTileDisplayHelper::vtkInternals
{
public:
  static vtkSmartPointer<vtkTileDisplayHelper> Instance;

  class vtkTile
    {
  public:
    vtkSynchronizedRenderers::vtkRawImage TileImage;

    vtkSmartPointer<vtkRenderer> Renderer;

    // PhysicalViewport is the viewport where the TileImage maps into the tile
    // rendered by this processes i.e. the render window for this process.
    double PhysicalViewport[4];
    };

  typedef vtkstd::map<void*, vtkTile> TilesMapType;
  TilesMapType TilesMap;

  void FlushTile(const TilesMapType::iterator& iter)
    {
    if (iter != this->TilesMap.end())
      {
      vtkTile& tile = iter->second;
      vtkRenderer* renderer = tile.Renderer;
      if (tile.TileImage.IsValid() && renderer)
        {
        double viewport[4];
        renderer->GetViewport(viewport);
        renderer->SetViewport(tile.PhysicalViewport);
        tile.TileImage.PushToViewport(renderer);
        renderer->SetViewport(viewport);
        }
      }
    }

  // Iterates over all valid tiles in the TilesMap and flush the images to the
  // screen.
  void FlushTiles(void* current)
    {
    for (TilesMapType::iterator iter = this->TilesMap.begin();
      iter != this->TilesMap.end(); ++iter)
      {
      if (iter->first != current)
        {
        this->FlushTile(iter);
        }
      }
    // Render the current tile last, this is done in case where user has
    // overlapping views. This ensures that active view is always rendered on
    // top.
    this->FlushTile(TilesMap.find(current));
    }

  void EraseTile(void* key)
    {
    TilesMapType::iterator iter = this->TilesMap.find(key);
    if (iter != this->TilesMap.end())
      {
      this->TilesMap.erase(iter);
      }
    }
};

vtkSmartPointer<vtkTileDisplayHelper>
vtkTileDisplayHelper::vtkInternals::Instance;

//----------------------------------------------------------------------------
vtkTileDisplayHelper* vtkTileDisplayHelper::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTileDisplayHelper");
  if(ret)
    {
    return static_cast<vtkTileDisplayHelper*>(ret);
    }
  return new vtkTileDisplayHelper;
}

//----------------------------------------------------------------------------
vtkTileDisplayHelper::vtkTileDisplayHelper()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkTileDisplayHelper::~vtkTileDisplayHelper()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkTileDisplayHelper* vtkTileDisplayHelper::GetInstance()
{
  if (!vtkInternals::Instance)
    {
    vtkInternals::Instance.TakeReference(vtkTileDisplayHelper::New());
    }
  return vtkInternals::Instance;
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::SetTile(void* key,
  double viewport[4], vtkRenderer* renderer,
  vtkSynchronizedRenderers::vtkRawImage& image)
{
  vtkInternals::vtkTile &tile = this->Internals->TilesMap[key];
  memcpy(tile.PhysicalViewport, viewport, 4*sizeof(double));
  tile.Renderer = renderer;
  tile.TileImage = image;
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::EraseTile(void* key)
{
  this->Internals->TilesMap.erase(key);
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::FlushTiles(void* key)
{
  this->Internals->FlushTiles(key);
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
