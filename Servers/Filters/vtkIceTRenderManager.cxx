/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTRenderManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkIceTRenderManager.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIceTContext.h"
#include "vtkIceTRenderer.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMPIController.h"
#include "vtkMPI.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPerspectiveTransform.h"
#include "vtkPKdTree.h"
#include "vtkProcessModule.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#include <GL/ice-t.h>

#include "vtkgl.h"
#include "assert.h"
#include <vtkstd/algorithm>

#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif

#define MY_MAX(x, y)    ((x) < (y) ? (y) : (x))
#define MY_MIN(x, y)    ((x) < (y) ? (x) : (y))

// ******************************************************************
// Callback commands.
// ******************************************************************
static void vtkIceTRenderManagerRecordIceTImage(vtkObject *caller,
                                                unsigned long,
                                                void *clientdata,
                                                void *)
{
  vtkIceTRenderer *renderer
    = reinterpret_cast<vtkIceTRenderer *>(caller);
  vtkIceTRenderManager *self
    = reinterpret_cast<vtkIceTRenderManager *>(clientdata);
  self->RecordIceTImage(renderer);
}

static void vtkIceTRenderManagerReconstructWindowImage(vtkObject *,
                                                       unsigned long,
                                                       void *clientdata,
                                                       void *)
{
  vtkIceTRenderManager *self
    = reinterpret_cast<vtkIceTRenderManager *>(clientdata);
  self->ForceImageWriteback();
}

//******************************************************************
// vtkIceTRenderManager implementation.
//******************************************************************

vtkStandardNewMacro(vtkIceTRenderManager);

vtkCxxSetObjectMacro(vtkIceTRenderManager, TileViewportTransform,
                     vtkPerspectiveTransform);

vtkIceTRenderManager::vtkIceTRenderManager()
{
  this->TileDimensions[0] = 1;
  this->TileDimensions[1] = 1;
  this->TileMullions[0] = 0;
  this->TileMullions[1] = 0;
  this->TileRanks = new int*[1];
  this->TileRanks[0] = new int[1];
  this->TileRanks[0][0] = 0;
  this->TilesDirty = 1;

  this->LastKnownImageReductionFactor = 0;

  this->TileViewportTransform = NULL;

  this->LastViewports = vtkDoubleArray::New();
  this->LastViewports->SetNumberOfComponents(4);
  this->LastViewports->SetNumberOfTuples(0);

  this->ReducedZBuffer = vtkFloatArray::New();

  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetClientData(this);
  cbc->SetCallback(vtkIceTRenderManagerRecordIceTImage);
  this->RecordIceTImageCallback = cbc;

  cbc = vtkCallbackCommand::New();
  cbc->SetClientData(this);
  cbc->SetCallback(vtkIceTRenderManagerReconstructWindowImage);
  this->FixRenderWindowCallback = cbc;

  // Reload the controller so that we make an ICE-T context.
  this->Superclass::SetController(NULL);
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->PhysicalViewport[0] = 
    this->PhysicalViewport[1] = 
    this->PhysicalViewport[2] = 
    this->PhysicalViewport[3];

  this->EnableTiles = 0;
}

//-------------------------------------------------------------------------

vtkIceTRenderManager::~vtkIceTRenderManager()
{
  this->SetController(NULL);
  for (int x = 0; x < this->TileDimensions[0]; x++)
    {
    delete[] this->TileRanks[x];
    }
  delete[] this->TileRanks;
  this->SetTileViewportTransform(NULL);

  this->RecordIceTImageCallback->Delete();
  this->FixRenderWindowCallback->Delete();
  this->LastViewports->Delete();
  this->ReducedZBuffer->Delete();
}

//-------------------------------------------------------------------------

vtkRenderer *vtkIceTRenderManager::MakeRenderer()
{
  return vtkIceTRenderer::New();
}

//-------------------------------------------------------------------------

void vtkIceTRenderManager::SetController(vtkMultiProcessController *controller)
{
  vtkDebugMacro("SetController to " << controller);

  if (controller == this->Controller)
    {
    return;
    }

  if (controller != NULL)
    {
    vtkCommunicator *communicator = controller->GetCommunicator();
    if (!communicator || (!communicator->IsA("vtkMPICommunicator")))
      {
      vtkErrorMacro("vtkIceTRenderManager parallel compositor currently works only with an MPI communicator.");
      return;
      }
    }

  this->Superclass::SetController(controller);
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetRenderWindow(vtkRenderWindow *renwin)
{
  if (this->RenderWindow == renwin)
    {
    return;
    }

  this->Superclass::SetRenderWindow(renwin);

  this->ContextDirty = 1;
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::UpdateIceTContext()
{
  vtkDebugMacro("UpdateIceTContext");

  if (this->ContextDirty || this->TilesDirty)
    {
    // Compute transform for non-ICE-T renderers.
    this->ComputeTileViewportTransform();
    }

  if (this->ImageReductionFactor != this->LastKnownImageReductionFactor)
    {
    this->TilesDirty = 1;
    this->LastKnownImageReductionFactor = this->ImageReductionFactor;
    }

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *ren;

  renderers->InitTraversal(cookie);
  while ((ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *icetRen = vtkIceTRenderer::SafeDownCast(ren);
    if (icetRen == NULL)
      {
      // Assuming this renderer will be rendered after all IceT renderers
      // (which can be assured if it is on a higher level), set up a callback
      // to fix the composited image before rendering occurs.
      ren->AddObserver(vtkCommand::StartEvent, this->FixRenderWindowCallback);
      continue;
      }
    icetRen->AddObserver(vtkCommand::EndEvent, this->RecordIceTImageCallback);

    icetRen->SetController(this->Controller);
    icetRen->GetContext()->MakeCurrent();

    if (   this->ContextDirty || this->TilesDirty
        || (this->CleanScreenWidth != this->FullImageSize[0])
        || (this->CleanScreenHeight != this->FullImageSize[1]) )
      {
      int x, y;

      double normalizedRendererViewport[4];
      icetRen->GetViewport(normalizedRendererViewport);

      int* tileScale = this->RenderWindow->GetTileScale();
      // displaySize should take into account render window's
      // tileScale. Later code assumes that the display size
      // includes it.
      // KEN: On reviewing this code, the calculation is a bit "weird."  Why is
      // the displaySize being calculated with the FullImageSize and then scaled
      // by ImageReductionFactor?  Why not just use the ReducedImageSize?  It
      // may also make sense to make the condition above based on the reduced
      // image size and get rid of the condition above it concerning the image
      // reduction factor.  There is also little point to using the render
      // window tile scale rather than the local TileDimensions ivar.
      int displaySize[2];
      displaySize[0] = tileScale[0]* this->FullImageSize[0];
      displaySize[1] = tileScale[1]* this->FullImageSize[1];
      displaySize[0] += int((this->TileDimensions[0]-1)*this->TileMullions[0]*this->ImageReductionFactor);
      displaySize[1] += int((this->TileDimensions[1]-1)*this->TileMullions[1]*this->ImageReductionFactor);

      // Adjust the global viewport of the renderer.  That is convert the
      // normalized renderer viewport (as values between 0 and 1) to actual
      // pixel values.
      int rendererViewport[4];
      rendererViewport[0]
        = (int)(displaySize[0]*normalizedRendererViewport[0] + 0.5);
      rendererViewport[1]
        = (int)(displaySize[1]*normalizedRendererViewport[1] + 0.5);
      rendererViewport[2]
        = (int)(displaySize[0]*normalizedRendererViewport[2] + 0.5);
      rendererViewport[3]
        = (int)(displaySize[1]*normalizedRendererViewport[3] + 0.5);

      icetRen->SetPhysicalViewport(0, 0, 0, 0);

      icetResetTiles();
      for (x = 0; x < this->TileDimensions[0]; x++)
        {
        for (y = 0; y < this->TileDimensions[1]; y++)
          {
          int tileViewport[4];
          this->GetTileViewport(x, y, tileViewport);

          // Check to see if this renderer intersects this tile.
          if (   (tileViewport[0] >= rendererViewport[2])
              || (tileViewport[2] <= rendererViewport[0])
              || (tileViewport[1] >= rendererViewport[3])
              || (tileViewport[3] <= rendererViewport[1]) )
            {
            continue;
            }

          // Intersect the tile viewport by the renderer's viewport.
          int visibleViewport[4];
          visibleViewport[0] = MY_MAX(tileViewport[0], rendererViewport[0]);
          visibleViewport[1] = MY_MAX(tileViewport[1], rendererViewport[1]);
          visibleViewport[2] = MY_MIN(tileViewport[2], rendererViewport[2]);
          visibleViewport[3] = MY_MIN(tileViewport[3], rendererViewport[3]);

          icetAddTile(visibleViewport[0],
                      visibleViewport[1],
                      visibleViewport[2]-visibleViewport[0],
                      visibleViewport[3]-visibleViewport[1],
                      this->TileRanks[x][y]);

          if (this->TileRanks[x][y] == this->Controller->GetLocalProcessId())
            {
            icetRen->SetPhysicalViewport(visibleViewport[0] - tileViewport[0],
                                         visibleViewport[1] - tileViewport[1],
                                         visibleViewport[2] - tileViewport[0],
                                         visibleViewport[3] - tileViewport[1]);
            }
          }
        }
      }

    if (this->ContextDirty || (this->MTime > this->ContextUpdateTime))
      {
      if (this->Debug)
        {
        icetDiagnostics(ICET_DIAG_DEBUG | ICET_DIAG_ALL_NODES);
        }
      else
        {
        icetDiagnostics(ICET_DIAG_WARNINGS | ICET_DIAG_ALL_NODES);
        }
      icetDisable(ICET_DISPLAY);
      icetEnable(ICET_CORRECT_COLORED_BACKGROUND);

      if (this->UseCompositing)
        {
        vtkIntArray *drg = icetRen->GetDataReplicationGroup();
        // Compiler, optimize this away.
        if (sizeof(int) == sizeof(GLint))
          {
          icetDataReplicationGroup(drg->GetNumberOfTuples(),
                                   (const GLint *)drg->GetPointer(0));
          }
        else
          {
          vtkIdType numtuples = drg->GetNumberOfTuples();
          const int *original_data = drg->GetPointer(0);
          GLint *new_data = new GLint[numtuples];
          vtkstd::copy(original_data, original_data + numtuples, new_data);
          icetDataReplicationGroup(numtuples, new_data);
          delete new_data;
          }
        }
      else
        {
        // If we're not compositing, tell ICE-T that all processes have
        // duplicated data.  ICE-T will just have each process render to its
        // local tile instead.
        GLint *drg = new GLint[this->Controller->GetNumberOfProcesses()];
        for (GLint i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
          {
          drg[i] = i;
          }
        icetDataReplicationGroup(this->Controller->GetNumberOfProcesses(), drg);
        delete[] drg;
        }
      }
    }

  this->TilesDirty = 0;
  this->CleanScreenWidth = this->FullImageSize[0];
  this->CleanScreenHeight = this->FullImageSize[1];

  this->ContextUpdateTime.Modified();
  this->ContextDirty = 0;
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetTileDimensions(int tilesX, int tilesY)
{
  vtkDebugMacro("SetTileDimensions " << tilesX << " " << tilesY);

  if (   (this->TileDimensions[0] == tilesX)
      && (this->TileDimensions[1] == tilesY))
    {
    return;
    }

  int x, y;
  int **NewTileRanks;

  NewTileRanks = new int*[tilesX];
  for (x = 0; x < tilesX; x++)
    {
    NewTileRanks[x] = new int[tilesY];
    for (y = 0; y < tilesY; y++)
      {
      if ( (y < this->TileDimensions[1]) && (x < this->TileDimensions[0]))
        {
        NewTileRanks[x][y] = this->TileRanks[x][y];
        }
      else
        {
        NewTileRanks[x][y] = y*tilesX + x;
        }
      }
    if (x < this->TileDimensions[0])
      {
      delete[] this->TileRanks[x];
      }
    }

  delete[] this->TileRanks;
  this->TileRanks = NewTileRanks;
  this->TileDimensions[0] = tilesX;
  this->TileDimensions[1] = tilesY;
  this->TilesDirty = 1;
}
//-----------------------------------------------------------------------------

int vtkIceTRenderManager::GetTileRank(int x, int y)
{
  vtkDebugMacro("GetTileRank " << x << " " << y);

  if (   (x < 0) || (x >= this->TileDimensions[0])
      || (y < 0) || (y >= this->TileDimensions[1]) )
    {
    vtkErrorMacro("Invalid tile " << x << ", " << y);
    return -1;
    }

  return this->TileRanks[x][y];
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetTileRank(int x, int y, int rank)
{
  vtkDebugMacro("SetTileRank " << x << " " << y << " " << rank);

  if (   (x < 0) || (x >= this->TileDimensions[0])
      || (y < 0) || (y >= this->TileDimensions[1]) )
    {
    vtkErrorMacro("Invalid tile " << x << ", " << y);
    return;
    }

  this->TileRanks[x][y] = rank;
  this->TilesDirty = 1;
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetTileMullions(int mullX, int mullY)
{
  this->TileMullions[0] = mullX;
  this->TileMullions[1] = mullY;
  this->TilesDirty = 1;
}

//-----------------------------------------------------------------------------
void vtkIceTRenderManager::SetEnableTiles(int enable)
{
  if (this->EnableTiles != enable)
    {
    this->EnableTiles = enable;
    this->TilesDirty = true;
    this->SetSynchronizeTileProperties(enable? 0 : 1);
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkIceTRenderManager::ComputeTileViewportTransform()
{
  vtkDebugMacro("ComputeTileViewportTransform");

  if (!this->EnableTiles)
    {
    return;
    }

  if (!this->Controller)
    {
    vtkDebugMacro("No controller, no viewport set.");
    return;
    }

  int rank = this->Controller->GetLocalProcessId();

  // Initialize tiled window for 2D actors.  We may not be displaying any
  // tile, in which case just show something.

  // The mullion size as a fraction of total display size.
  // Here we are assuming that each of the tiles is of the same size.
  double mullions[2];
  mullions[0] = static_cast<double>(this->TileMullions[0])/
    (this->FullImageSize[0] * this->TileDimensions[0]);
  mullions[1] = static_cast<double>(this->TileMullions[1])/
    (this->FullImageSize[1] * this->TileDimensions[1]);

  // The size of the tile as a fraction of the total display size.
  double tileSize[2];
  tileSize[0] = 1.0/(double)this->TileDimensions[0];
  tileSize[1] = 1.0/(double)this->TileDimensions[1];

  // The spacing of the tiles (including the mullions).
  double tileSpacing[2];
  tileSpacing[0] = tileSize[0] + mullions[0];
  tileSpacing[1] = tileSize[1] + mullions[1];

  this->RenderWindow->SetTileScale(this->TileDimensions);
  this->RenderWindow->SetTileViewport(0.0, 0.0, tileSize[0], tileSize[1]);
  
  for (int y = 0; y < this->TileDimensions[1]; y++)
    {
    for (int x = 0; x < this->TileDimensions[0]; x++)
      {
      if (this->TileRanks[x][y] == rank)
        {
        // Transform camera for 3D actors.
        vtkPerspectiveTransform *transform = vtkPerspectiveTransform::New();
        transform->Identity();
        transform->Ortho(2.0*(x*tileSpacing[0]) - 1.0,
                         2.0*(x*tileSpacing[0] + tileSize[0]) - 1.0,
                         2.0*(y*tileSpacing[1]) - 1.0,
                         2.0*(y*tileSpacing[1] + tileSize[1]) - 1.0,
                         1.0, -1.0);
        this->SetTileViewportTransform(transform);
        transform->Delete();

        // Establish tiled window for 2D actors.
        if (this->RenderWindow)
          {
          // RenderWindow tiles from lower left instead of upper left.
          y = this->TileDimensions[1] - y - 1;
          this->RenderWindow->SetTileViewport(x*tileSpacing[0],
                                              y*tileSpacing[1],
                                              x*tileSpacing[0]+tileSize[0],
                                              y*tileSpacing[1]+tileSize[1]);
          }

        return;
        }
      }
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetStrategy(int strategy)
{
  vtkDebugMacro("SetStrategy to " << strategy);

  if (!this->RenderWindow)
    {
    vtkErrorMacro("Must set the render window and its renderers before calling SetStrategy.");
    return;
    }

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *_ren;

  renderers->InitTraversal(cookie);
  while ((_ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
    if (!ren) continue;

    ren->SetStrategy(strategy);
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetStrategy(const char *strategy)
{
  vtkDebugMacro("SetStrategy to " << strategy);

  if (strcmp(strategy, "DEFAULT") == 0)
    {
    this->SetStrategy(DEFAULT);
    }
  else if (strcmp(strategy, "REDUCE") == 0)
    {
    this->SetStrategy(REDUCE);
    }
  else if (strcmp(strategy, "VTREE") == 0)
    {
    this->SetStrategy(VTREE);
    }
  else if (strcmp(strategy, "SPLIT") == 0)
    {
    this->SetStrategy(SPLIT);
    }
  else if (strcmp(strategy, "SERIAL") == 0)
    {
    this->SetStrategy(SERIAL);
    }
  else if (strcmp(strategy, "DIRECT") == 0)
    {
    this->SetStrategy(DIRECT);
    }
  else
    {
    vtkWarningMacro("No such strategy " << strategy);
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetComposeOperation(int operation)
{
  vtkDebugMacro("SetComposeOperation to " << operation);

  if (!this->RenderWindow)
    {
    vtkErrorMacro("Must set the render window and its renderers before calling SetComposeOperation.");
    return;
    }

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *_ren;

  renderers->InitTraversal(cookie);
  while ((_ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
    if (!ren) continue;

    ren->SetComposeOperation(operation);
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetSortingKdTree(vtkPKdTree *tree)
{
  vtkDebugMacro("SetSortingKdTree to " << tree);

  if (!this->RenderWindow)
    {
    vtkErrorMacro("Must set the render window and its renderers before calling SetSortingKdTree.");
    return;
    }

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *_ren;

  renderers->InitTraversal(cookie);
  while ((_ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
    if (!ren) continue;

    ren->SetSortingKdTree(tree);
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetDataReplicationGroup(vtkIntArray *group)
{
  vtkDebugMacro("SetDataReplicationGroup to " << group);

  if (!this->RenderWindow)
    {
    vtkErrorMacro("Must set the render window and its renderers before calling SetComposeOperation.");
    return;
    }

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *_ren;

  renderers->InitTraversal(cookie);
  while ((_ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
    if (!ren) continue;

    ren->SetDataReplicationGroup(group);
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetDataReplicationGroupColor(int color)
{
  vtkDebugMacro("SetDataReplicationGroupColor to " << color);

  if (!this->RenderWindow)
    {
    vtkErrorMacro("Must set the render window and its renderers before calling SetComposeOperation.");
    return;
    }

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *_ren;

  renderers->InitTraversal(cookie);
  while ((_ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
    if (!ren) continue;

    ren->SetDataReplicationGroupColor(color);
    }
}

//-----------------------------------------------------------------------------

double vtkIceTRenderManager::GetRenderTime()
{
  double t = 0.0;

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *_ren;

  renderers->InitTraversal(cookie);
  while ((_ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
    if (ren)
      {
      t += ren->GetRenderTime();
      }
    else
      {
      t += _ren->GetLastRenderTimeInSeconds();
      }
    }
  return t;
}  

//-----------------------------------------------------------------------------

double vtkIceTRenderManager::GetImageProcessingTime()
{
  double t = this->ImageProcessingTime;

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *_ren;

  renderers->InitTraversal(cookie);
  while ((_ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
    if (!ren) continue;

    t += ren->GetBufferReadTime();
    }
  return t;
}  

//-----------------------------------------------------------------------------

double vtkIceTRenderManager::GetBufferReadTime()
{
  double t = 0.0;

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *_ren;

  renderers->InitTraversal(cookie);
  while ((_ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
    if (!ren) continue;

    t += ren->GetBufferReadTime();
    }
  return t;
}  

//-----------------------------------------------------------------------------

double vtkIceTRenderManager::GetBufferWriteTime()
{
  double t = 0.0;

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *_ren;

  renderers->InitTraversal(cookie);
  while ((_ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
    if (!ren) continue;

    t += ren->GetBufferWriteTime();
    }
  return t;
}  

//-----------------------------------------------------------------------------

double vtkIceTRenderManager::GetCompositeTime()
{
  double t = 0.0;

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *_ren;

  renderers->InitTraversal(cookie);
  while ((_ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
    if (!ren) continue;

    t += ren->GetCompositeTime();
    }
  return t;
}

//-----------------------------------------------------------------------------
void vtkIceTRenderManager::CollectWindowInformation(vtkMultiProcessStream& stream)
{
  vtkDebugMacro("Sending Window Information");

  this->Superclass::CollectWindowInformation(stream);

  // insert the tag to ensure we reading back the correct information.
  stream << vtkProcessModule::IceTWinInfo;
  stream << this->TilesDirty;
  if (this->TilesDirty)
    {
    stream << this->TileDimensions[0] << this->TileDimensions[1];
    for (int x = 0; x < this->TileDimensions[0]; x++)
      {
      for (int y=0; y < this->TileDimensions[1]; y++)
        {
        stream << (this->TileRanks[x])[y];
        }
      }
    }
  stream << vtkProcessModule::IceTWinInfo;
}

//-----------------------------------------------------------------------------

bool vtkIceTRenderManager::ProcessWindowInformation(vtkMultiProcessStream& stream)
{
  vtkDebugMacro("Receiving Window Information");

  if (!this->Superclass::ProcessWindowInformation(stream))
    {
    return false;
    }

  int tag;
  stream >> tag;
  if (tag != vtkProcessModule::IceTWinInfo)
    {
    vtkErrorMacro("Incorrect tag received. Aborting for debugging purposes.");
    return false;
    }

  int tilesDirty;
  stream >> tilesDirty;
  if (tilesDirty)
    {
    int newNumTilesX, newNumTilesY;
    stream >> newNumTilesX >> newNumTilesY;
    this->SetTileDimensions(newNumTilesX, newNumTilesY);
    for (int x = 0; x < this->TileDimensions[0]; x++)
      {
      for (int y=0; y < this->TileDimensions[1]; y++)
        {
        stream >> (this->TileRanks[x])[y];
        }
      }
    }
  stream >> tag;
  if (tag != vtkProcessModule::IceTWinInfo)
    {
    vtkErrorMacro("Incorrect tag received. Aborting for debugging purposes.");
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkIceTRenderManager::CollectRendererInformation(vtkRenderer *_ren,
  vtkMultiProcessStream& stream)
{
  vtkDebugMacro("Sending renderer information for " << _ren);

  this->Superclass::CollectRendererInformation(_ren, stream);

  vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
  if (!ren)
    {
    return;
    }

  stream << ren->GetStrategy()
         << ren->GetComposeOperation();
}

//-----------------------------------------------------------------------------
bool vtkIceTRenderManager::ProcessRendererInformation(vtkRenderer *_ren,
  vtkMultiProcessStream& stream)
{
  vtkDebugMacro("Receiving renderer information for " << _ren);

  if (!this->Superclass::ProcessRendererInformation(_ren, stream))
    {
    return false;
    }

  vtkIceTRenderer *ren = vtkIceTRenderer::SafeDownCast(_ren);
  if (ren) 
    {
    int strategy;
    int compose_operation;
    stream >> strategy >> compose_operation;
    ren->SetStrategy(strategy);
    ren->SetComposeOperation(compose_operation);
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkIceTRenderManager::PreRenderProcessing()
{
  vtkDebugMacro("PreRenderProcessing");
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkRendererCollection *rens = renWin->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer* ren;
  int i;

  // Normally if this->UseCompositing was false, we would return here.  But
  // if we did that, the tile display would become invalid.  Instead, in
  // UpdateIceTContext we tell ICE-T that the data is replicated on all
  // nodes.  This will make ICE-T turn off compositing and make each
  // process render its local tile (if any).  The only real overhead is an
  // unnecessary frame buffer read back by ICE-T.  If that's a problem, we
  // could always add a special case to ICE-T...

  // Make sure the viewports have not changed.
  if (this->LastViewports->GetNumberOfTuples() != rens->GetNumberOfItems())
    {
    this->LastViewports->SetNumberOfTuples(rens->GetNumberOfItems());
    this->TilesDirty = 1;
    }
  if (!this->TilesDirty)
    {
    for (rens->InitTraversal(cookie), i = 0;
         (ren = rens->GetNextRenderer(cookie)) != NULL; i++)
      {
      double *lastViewport, *viewport;
      lastViewport = this->LastViewports->GetTuple(i);
      viewport = ren->GetViewport();
      if (   (lastViewport[0] != viewport[0])
          || (lastViewport[1] != viewport[1])
          || (lastViewport[2] != viewport[2])
          || (lastViewport[3] != viewport[3]) )
        {
        this->TilesDirty = 1;
        }
      }
    }

  this->UpdateIceTContext();

  if (rens->GetNumberOfItems() == 0)
    {
    vtkErrorMacro("Missing renderer.");
    return;
    }

  int foundIceTRenderer = 0;

  for (rens->InitTraversal(cookie), i = 0;
       (ren = rens->GetNextRenderer(cookie)) != NULL; i++)
    {
    this->LastViewports->SetTuple(i, ren->GetViewport());

    vtkIceTRenderer *icetRen = vtkIceTRenderer::SafeDownCast(ren);
    if (icetRen == NULL)
      {
      // For all non-IceT renderers, assume that the data is replicated, do no
      // compositing, and asjust the camera to focus on the displayed tile.
      ren->GetActiveCamera()
        ->SetUserTransform(this->GetTileViewportTransform());
      continue;
      }

    foundIceTRenderer = 1;

    if (!icetRen->GetDraw())
      {
      // This renderer not being drawn.  Skip it.
      continue;
      }

    icetRen->SetComposeNextFrame(1);
    }

  if (!foundIceTRenderer)
    {
    vtkWarningMacro("vtkIceTRenderManager used with renderer that is not "
                    "vtkIceTRenderer.\n"
                    "Remember to use\n\n"
                    "    vtkParallelRenderManager::MakeRenderer()\n\n"
                    "in place of vtkRenderer::New()");
    }

  // We will be updating the reduced image in renderer callbacks.  By the time
  // the render window completely finishes, the image should be completely up to
  // date.  Also, if some renderers were shut off, we need to use cached images
  // from the last render.  Go a head and just say we have valid images.
  this->ReducedImageUpToDate = 1;
  if (this->MagnifyImages && this->WriteBackImages)
    {
    // We will also magnify the images as we receive them.
    this->FullImageUpToDate = 1;
    this->FullImage->SetNumberOfComponents(4);
    this->FullImage->SetNumberOfTuples(  this->FullImageSize[0]
                                       * this->FullImageSize[1]);
    }

  if (this->ImageReductionFactor == 1)
    {
    // Share the two image buffers.
    this->FullImage->SetNumberOfComponents(4);
    this->FullImage->SetNumberOfTuples(  this->FullImageSize[0]
                                       * this->FullImageSize[1]);
    this->ReducedImage->SetArray(this->FullImage->GetPointer(0),
                                 4*this->FullImageSize[0]
                                  *this->FullImageSize[1], 1);
    this->FullImageUpToDate = 1;
    }

  this->ReducedImage->SetNumberOfComponents(4);
  this->ReducedImage->SetNumberOfTuples(  this->ReducedImageSize[0]
                                        * this->ReducedImageSize[1]);

  // Turn swap buffers off before the render so the end render method has a
  // chance to add to the back buffer.
  if (this->UseBackBuffer)
    {
    this->RenderWindow->SwapBuffersOff();
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::PostRenderProcessing()
{
  vtkDebugMacro("PostRenderProcessing");
  vtkTimerLog::MarkStartEvent("Compositing");

  this->Controller->Barrier();

  vtkRendererCollection *renderers = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *ren;
  renderers->InitTraversal(cookie);
  while ((ren = renderers->GetNextRenderer(cookie)) != NULL)
    {
    ren->RemoveObservers(vtkCommand::EndEvent, this->RecordIceTImageCallback);
    ren->RemoveObservers(vtkCommand::StartEvent, this->FixRenderWindowCallback);
    }

  this->WriteFullImage();

  // Swap buffers here.
  if (this->UseBackBuffer)
    {
    this->RenderWindow->SwapBuffersOn();
    }
  this->RenderWindow->Frame();

  vtkTimerLog::MarkEndEvent("Compositing");
}

//-----------------------------------------------------------------------------
void vtkIceTRenderManager::RecordIceTImage(vtkIceTRenderer *icetRen)
{
  int physicalViewport[4];
  icetRen->GetPhysicalViewport(physicalViewport);
  int width  = physicalViewport[2] - physicalViewport[0];
  int height = physicalViewport[3] - physicalViewport[1];

  // See if this renderer is displaying anything in this tile.
  if ((width < 1) || (height < 1)) return;

  // Yeah, this screws up the render timing for the superclass.  But if you look
  // at my implementation for GetRenderTime, you'll see that that measurement is
  // not used.
  this->Timer->StartTimer();

  icetRen->GetContext()->MakeCurrent();

  GLint color_format;
  icetGetIntegerv(ICET_COLOR_FORMAT, &color_format);

  if (color_format == GL_RGBA)
    {
    this->ReducedImage->SetNumberOfComponents(4);
    this->ReducedImage->SetNumberOfTuples(  this->ReducedImageSize[0]
                                          * this->ReducedImageSize[1]);

    // Copy as 4-bytes.  It's faster.
    GLuint *dest = (GLuint *)this->ReducedImage->WritePointer(
                      0, 4*this->ReducedImageSize[0]*this->ReducedImageSize[1]);
    GLuint *src = (GLuint *)icetGetColorBuffer();
    dest += physicalViewport[1]*this->ReducedImageSize[0];
    for (int j = 0; j < height; j++)
      {
      dest += physicalViewport[0];
      for (int i = 0; i < width; i++)
        {
        dest[0] = src[0];
        dest++;  src++;
        }
      dest += (this->ReducedImageSize[0] - physicalViewport[2]);
      }
    }
  else if ((GLenum)color_format == vtkgl::BGRA)
    {
    this->ReducedImage->SetNumberOfComponents(4);
    this->ReducedImage->SetNumberOfTuples(  this->ReducedImageSize[0]
                                          * this->ReducedImageSize[1]);
    // Note: you could probably speed this up by copying as 4-bytes and
    // doing integer arithmetic to convert from BGRA to RGBA.
    unsigned char *dest
      = this->ReducedImage->WritePointer(0,
                                         4*this->ReducedImageSize[0]
                                         *this->ReducedImageSize[1]);
    unsigned char *src = icetGetColorBuffer();
    dest += 4*physicalViewport[1]*this->ReducedImageSize[0];
    for (int j = 0; j < height; j++)
      {
      dest += 4*physicalViewport[0];
      for (int i = 0; i < width; i++)
        {
        dest[0] = src[2];
        dest[1] = src[1];
        dest[2] = src[0];
        dest[3] = src[3];
        dest += 4;  src += 4;
        }
      dest += 4*(this->ReducedImageSize[0] - physicalViewport[2]);
      }
    }
  else
    {
    vtkErrorMacro("ICE-T using unknown image format.");
    return;
    }

  if (icetRen->GetCollectDepthBuffer())
    {
    memcpy(this->PhysicalViewport, physicalViewport, 4*sizeof(int));
    
    // Get depth buffer.
    GLuint *zbuffer = (GLuint *)icetGetDepthBuffer();
    if (zbuffer)
      {
      this->ReducedZBuffer->SetNumberOfComponents(1);
      this->ReducedZBuffer->SetNumberOfTuples(width*height);
      const float divisor = pow((float)2, 32) -1;
      for (vtkIdType cc=0; cc < width* height; cc++)
        {
        this->ReducedZBuffer->SetValue(cc, zbuffer[cc]/divisor);
        }
      }
    }
  else if (this->ReducedZBuffer->GetNumberOfTuples() > 0)
    {
    this->ReducedZBuffer->Initialize();
    }
  

  this->Timer->StopTimer();
  this->ImageProcessingTime += this->Timer->GetElapsedTime();

  if (this->FullImage->GetPointer(0) != this->ReducedImage->GetPointer(0))
    {
    int fullImageViewport[4];
    fullImageViewport[0] =(int)(physicalViewport[0]*this->ImageReductionFactor);
    fullImageViewport[1] =(int)(physicalViewport[1]*this->ImageReductionFactor);
    fullImageViewport[2] =(int)(physicalViewport[2]*this->ImageReductionFactor);
    fullImageViewport[3] =(int)(physicalViewport[3]*this->ImageReductionFactor);

    // In case of rounding error, make sure the renderers are resized to the
    // full window.
    if (  (this->FullImageSize[0] - fullImageViewport[2])
        < this->ImageReductionFactor )
      {
      fullImageViewport[2] = this->FullImageSize[0];
      }
    if (  (this->FullImageSize[1] - fullImageViewport[3])
        < this->ImageReductionFactor )
      {
      fullImageViewport[3] = this->FullImageSize[1];
      }

    this->Timer->StartTimer();
    this->MagnifyImage(this->FullImage, this->FullImageSize,
                       this->ReducedImage, this->ReducedImageSize,
                       fullImageViewport, physicalViewport);
    }
}

//-----------------------------------------------------------------------------
// NOTE x,y are relative to the corner of the most recently rendered
// view module (this is significant is case of multiviews).
float vtkIceTRenderManager::GetZBufferValue(int x, int y)
{
  if (this->PhysicalViewport[0] == -1)
    {
    return 1.0f;
    }

  int width = this->PhysicalViewport[2]-this->PhysicalViewport[0];
  int height = this->PhysicalViewport[3] - this->PhysicalViewport[1];
  
  if (x>=0 && y>=0 && x <width && y < height)
    {
    int index = y* (width) + x;
    if (index < this->ReducedZBuffer->GetNumberOfTuples())
      {
      return this->ReducedZBuffer->GetValue(index);
      }
    }

  return 1.0f;
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::ForceImageWriteback()
{
  vtkDebugMacro("Forcing image writeback.");

  this->ReadReducedImage();

  int SaveWriteBackImages = this->WriteBackImages;
  this->WriteBackImages = 1;
  this->WriteFullImage();
  this->WriteBackImages = SaveWriteBackImages;
}

//-----------------------------------------------------------------------------

int vtkIceTRenderManager::ImageReduceRenderer(vtkRenderer *ren)
{
  return ren->IsA("vtkIceTRenderer");
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::GetGlobalViewport(int viewport[4])
{
  viewport[0] = viewport[1] = 0;
  viewport[2] = this->TileDimensions[0]
    * (  this->ReducedImageSize[0]
       + (int)(this->TileMullions[0]/this->ImageReductionFactor) );
  viewport[3] = this->TileDimensions[1]
    * (  this->ReducedImageSize[1]
       + (int)(this->TileMullions[1]/this->ImageReductionFactor) );
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::GetTileViewport(int x, int y, int viewport[4])
{
  // "Invert" y so that tiles are counted from top to bottom.
  y = this->TileDimensions[1]-y-1;
  viewport[0] = x*(  this->ReducedImageSize[0]
                   + (int)(this->TileMullions[0]/this->ImageReductionFactor) );
  viewport[1] = y*(  this->ReducedImageSize[1]
                   + (int)(this->TileMullions[1]/this->ImageReductionFactor) );

  viewport[2] = viewport[0] + this->ReducedImageSize[0];
  viewport[3] = viewport[1] + this->ReducedImageSize[1];
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::PrintSelf(ostream &os, vtkIndent indent)
{
  int x, y;

  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableTiles: " 
    << (this->EnableTiles? "On" : "Off") << endl;
  os << indent << "Display: " << this->TileDimensions[0]
     << " X " << this->TileDimensions[1] << " with display ranks" << endl;
  vtkIndent rankIndent = indent.GetNextIndent();
  for (y = 0; y < this->TileDimensions[1]; y++)
    {
    os << rankIndent;
    for (x = 0; x < this->TileDimensions[0]; x++)
      {
      os.width(4);
      os << this->GetTileRank(x, y);
      }
    os << endl;
    }
  os.width(0);
  os << indent << "Mullions: " << this->TileMullions[0] << ", "
     << this->TileMullions[1] << endl;
}
