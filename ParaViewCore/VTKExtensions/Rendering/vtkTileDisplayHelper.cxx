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

#include "vtkCamera.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

#include <sstream>
#include <vtkNew.h>

#include <map>
#include <set>
#include <string>

class vtkTileDisplayHelper::vtkInternals
{
public:
  std::string DumpImagePath;
  static vtkSmartPointer<vtkTileDisplayHelper> Instance;
  vtkNew<vtkPNGWriter> PNGWriter;

  class vtkTile
  {
  public:
    vtkSynchronizedRenderers::vtkRawImage TileImage;

    vtkSmartPointer<vtkRenderer> Renderer;

    // PhysicalViewport is the viewport where the TileImage maps into the tile
    // rendered by this processes i.e. the render window for this process.
    double PhysicalViewport[4];
  };

  typedef std::set<unsigned int> KeySet;
  KeySet EnabledKeys;

  typedef std::map<unsigned int, vtkTile> TilesMapType;
  TilesMapType LeftEyeTilesMap;
  TilesMapType RightEyeTilesMap;

  void FlushTile(
    const TilesMapType::iterator& iter, const TilesMapType& TileMap, const int& vtkNotUsed(leftEye))
  {
    if (iter != TileMap.end() && (this->EnabledKeys.size() == 0 ||
                                   this->EnabledKeys.find(iter->first) != this->EnabledKeys.end()))
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
  void FlushTiles(unsigned int current, const int& leftEye)
  {
    TilesMapType* TileMap = NULL;
    if (leftEye)
    {
      TileMap = &this->LeftEyeTilesMap;
    }
    else
    {
      TileMap = &this->RightEyeTilesMap;
    }
    for (TilesMapType::iterator iter = TileMap->begin(); iter != TileMap->end(); ++iter)
    {
      if (iter->first != current)
      {
        this->FlushTile(iter, *TileMap, leftEye);
      }
    }
    // Render the current tile last, this is done in case where user has
    // overlapping views. This ensures that active view is always rendered on
    // top.
    this->FlushTile(TileMap->find(current), *TileMap, leftEye);

    // Check if dumping the tile as an image is needed
    if (!vtkTileDisplayHelper::GetInstance()->Internals->DumpImagePath.empty())
    {
      for (TilesMapType::iterator iter = TileMap->begin(); iter != TileMap->end(); ++iter)
      {
        vtkTile& tile = iter->second;
        if (tile.Renderer)
        {
          vtkSynchronizedRenderers::vtkRawImage rawImage;
          double viewport[4];
          tile.Renderer->GetViewport(viewport);
          tile.Renderer->SetViewport(0, 0, 1, 1);
          rawImage.Capture(tile.Renderer);
          tile.Renderer->SetViewport(viewport);

          // Save RGBA raw image into a RGB PNG file
          if (rawImage.IsValid())
          {
            // Allocate RGB output
            vtkNew<vtkImageData> imageBuffer;
            imageBuffer->SetDimensions(rawImage.GetWidth(), rawImage.GetHeight(), 1);
            imageBuffer->AllocateScalars(VTK_UNSIGNED_CHAR, 3); // RGB

            // Copy RGBA image to a RBG one.
            unsigned char* readRGBAPointer =
              (unsigned char*)rawImage.GetRawPtr()->GetVoidPointer(0);
            unsigned char* writeRGBPointer = (unsigned char*)imageBuffer->GetScalarPointer();
            vtkIdType nbTuples = rawImage.GetWidth() * rawImage.GetHeight();
            for (vtkIdType tupleIndex = 0; tupleIndex < nbTuples; ++tupleIndex)
            {
              memcpy(writeRGBPointer, readRGBAPointer, 3); // Copy RBG
              writeRGBPointer += 3;                        // Skip RGB
              readRGBAPointer += 4;                        // Skip RGBA
            }

            this->PNGWriter->SetFileName(
              vtkTileDisplayHelper::GetInstance()->Internals->DumpImagePath.c_str());
            this->PNGWriter->SetInputData(imageBuffer.GetPointer());
            this->PNGWriter->Write();
          }
          break;
        }
      }
    }
  }
};

vtkSmartPointer<vtkTileDisplayHelper> vtkTileDisplayHelper::vtkInternals::Instance;

vtkStandardNewMacro(vtkTileDisplayHelper);
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
void vtkTileDisplayHelper::SetTile(unsigned int key, double viewport[4], vtkRenderer* renderer,
  vtkSynchronizedRenderers::vtkRawImage& image)
{
  vtkInternals::vtkTile* tile = NULL;
  if (renderer->GetActiveCamera()->GetLeftEye())
  {
    tile = &this->Internals->LeftEyeTilesMap[key];
  }
  else
  {
    tile = &this->Internals->RightEyeTilesMap[key];
  }

  memcpy(tile->PhysicalViewport, viewport, 4 * sizeof(double));
  tile->Renderer = renderer;
  tile->TileImage = image;
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::EraseTile(unsigned int key)
{
  this->Internals->LeftEyeTilesMap.erase(key);
  this->Internals->RightEyeTilesMap.erase(key);
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::EraseTile(unsigned int key, int leftEye)
{
  if (leftEye)
  {
    this->Internals->LeftEyeTilesMap.erase(key);
  }
  else
  {
    this->Internals->RightEyeTilesMap.erase(key);
  }
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::FlushTiles(unsigned int key, int leftEye)
{
  this->Internals->FlushTiles(key, leftEye);
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::ResetEnabledKeys()
{
  this->Internals->EnabledKeys.clear();
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::EnableKey(unsigned int key)
{
  this->Internals->EnabledKeys.insert(key);
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkTileDisplayHelper::SetDumpImagePath(const char* newPath)
{
  if (newPath == NULL)
  {
    vtkTileDisplayHelper::GetInstance()->Internals->DumpImagePath = "";
  }
  else
  {
    int pid = vtkMultiProcessController::GetGlobalController()
      ? vtkMultiProcessController::GetGlobalController()->GetLocalProcessId()
      : 1;

    std::ostringstream fullPath;
    fullPath << newPath << "-Tile" << pid << ".png";
    vtkTileDisplayHelper::GetInstance()->Internals->DumpImagePath = fullPath.str();
  }
}
