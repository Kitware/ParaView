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

#include "vtkMath.h"
#include "vtkIceTRenderManager.h"
#include "vtkIceTRenderer.h"
#include "vtkPKdTree.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkMPIController.h"
#include "vtkMPI.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCamera.h"
#include "vtkPerspectiveTransform.h"

#include <GL/ice-t.h>
#include <GL/ice-t_mpi.h>

#include <vtkstd/algorithm>

//******************************************************************
// Hidden structures.
//******************************************************************
struct IceTInformation {
  int TilesDirty;
  int Strategy;
  int ComposeOperation;
};
const int ICET_INFO_SIZE = sizeof(struct IceTInformation)/sizeof(int);

class vtkIceTRenderManagerOpaqueContext
{
public:
  IceTContext Handle;
};

//******************************************************************
// vtkIceTRenderManager implementation.
//******************************************************************

vtkCxxRevisionMacro(vtkIceTRenderManager, "1.24");
vtkStandardNewMacro(vtkIceTRenderManager);

vtkCxxSetObjectMacro(vtkIceTRenderManager, SortingKdTree, vtkPKdTree);
vtkCxxSetObjectMacro(vtkIceTRenderManager, DataReplicationGroup, vtkIntArray);
vtkCxxSetObjectMacro(vtkIceTRenderManager, TileViewportTransform,
                     vtkPerspectiveTransform);

vtkIceTRenderManager::vtkIceTRenderManager()
{
  this->Context = new vtkIceTRenderManagerOpaqueContext;

  this->TileDimensions[0] = 1;
  this->TileDimensions[1] = 1;
  this->TileRanks = new int*[1];
  this->TileRanks[0] = new int[1];
  this->TileRanks[0][0] = 0;
  this->TilesDirty = 1;

  this->Strategy = DEFAULT;
  this->StrategyDirty = 1;

  this->ComposeOperation = CLOSEST;
  this->ComposeOperationDirty = 1;

  this->SortingKdTree = NULL;

  this->LastKnownImageReductionFactor = 0;

  this->FullImageSharesData = 0;
  this->ReducedImageSharesData = 0;

  this->DataReplicationGroup = NULL;

  this->TileViewportTransform = NULL;

  // Reload the controller so that we make an ICE-T context.
  this->Superclass::SetController(NULL);
  this->SetController(vtkMultiProcessController::GetGlobalController());
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
  this->SetSortingKdTree(NULL);
  this->SetDataReplicationGroup(NULL);
  this->SetTileViewportTransform(NULL);

  delete this->Context;
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

  vtkCommunicator *communicator = NULL;
  if (controller != NULL)
    {
    communicator = controller->GetCommunicator();
    if (!communicator || (!communicator->IsA("vtkMPICommunicator")))
      {
      vtkErrorMacro("vtkIceTRenderManager parallel compositor currently works only with an MPI communicator.");
      return;
      }
    }

  if (this->Controller)
    {
    icetDestroyContext(this->Context->Handle);
    }

  this->Superclass::SetController(controller);

  if (this->Controller)
    {
    vtkMPICommunicator *MPICommunicator;
    MPICommunicator = vtkMPICommunicator::SafeDownCast(communicator);
    MPI_Comm handle = *MPICommunicator->GetMPIComm()->GetHandle();
    IceTCommunicator icet_comm = icetCreateMPICommunicator(handle);
    this->Context->Handle = icetCreateContext(icet_comm);
    icetDestroyMPICommunicator(icet_comm);
    vtkDebugMacro("Created new ICE-T context.");

    vtkIntArray *drg = vtkIntArray::New();
    drg->SetNumberOfComponents(1);
    drg->SetNumberOfTuples(1);
    drg->SetValue(0, this->Controller->GetLocalProcessId());
    this->SetDataReplicationGroup(drg);
    drg->Delete();
    }
  else
    {
    this->SetDataReplicationGroup(NULL);
    }

  this->ContextDirty = 1;
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::UpdateIceTContext()
{
  vtkDebugMacro("UpdateIceTContext");

  icetSetContext(this->Context->Handle);

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

  if (   this->ContextDirty || this->TilesDirty
      || (this->CleanScreenWidth != this->FullImageSize[0])
      || (this->CleanScreenHeight != this->FullImageSize[1]) )
    {
    int x, y;

    icetResetTiles();
    for (x = 0; x < this->TileDimensions[0]; x++)
      {
      for (y = 0; y < this->TileDimensions[1]; y++)
        {
        icetAddTile(x*this->ReducedImageSize[0],
                    (this->TileDimensions[1]-y-1)*this->ReducedImageSize[1],
                    this->ReducedImageSize[0], this->ReducedImageSize[1],
                    this->TileRanks[x][y]);
        }
      }
    this->TilesDirty = 0;
    this->CleanScreenWidth = this->FullImageSize[0];
    this->CleanScreenHeight = this->FullImageSize[1];
    }

  if (this->ContextDirty || this->StrategyDirty)
    {
    switch (this->Strategy)
      {
      case DEFAULT: icetStrategy(ICET_STRATEGY_REDUCE);  break;
      case REDUCE:  icetStrategy(ICET_STRATEGY_REDUCE);  break;
      case VTREE:   icetStrategy(ICET_STRATEGY_VTREE);   break;
      case SPLIT:   icetStrategy(ICET_STRATEGY_SPLIT);   break;
      case SERIAL:  icetStrategy(ICET_STRATEGY_SERIAL);  break;
      case DIRECT:  icetStrategy(ICET_STRATEGY_DIRECT);  break;
      default: vtkErrorMacro("Invalid strategy set"); break;
      }
    this->StrategyDirty = 0;
    }

  if (this->ContextDirty || this->ComposeOperationDirty)
    {
    switch (this->ComposeOperation)
      {
      case CLOSEST:
        icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT | ICET_DEPTH_BUFFER_BIT,
                               ICET_COLOR_BUFFER_BIT);
        break;
      case OVER:
        icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT, ICET_COLOR_BUFFER_BIT);
        break;
      default:
        vtkErrorMacro("Invalid compose operation set");
        break;
      }
    this->ComposeOperationDirty = 0;
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
    icetEnable(ICET_DISPLAY_COLORED_BACKGROUND);
    if (this->WriteBackImages)
      {
      icetEnable(ICET_DISPLAY);
      }
    else
      {
      icetDisable(ICET_DISPLAY);
      }
    if (this->MagnifyImages)
      {
      icetEnable(ICET_DISPLAY_INFLATE);
      }
    else
      {
      icetDisable(ICET_DISPLAY_INFLATE);
      }
    icetEnable(ICET_CORRECT_COLORED_BACKGROUND);

    if (this->UseCompositing)
      {
      // Compiler, optimize this away.
      if (sizeof(int) == sizeof(GLint))
        {
        icetDataReplicationGroup(
                      this->DataReplicationGroup->GetNumberOfTuples(),
                      (const GLint *)this->DataReplicationGroup->GetPointer(0));
        }
      else
        {
        vtkIdType numtuples = this->DataReplicationGroup->GetNumberOfTuples();
        const int *original_data = this->DataReplicationGroup->GetPointer(0);
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

void vtkIceTRenderManager::ComputeTileViewportTransform()
{
  vtkDebugMacro("ComputeTileViewportTransform");

  if (!this->Controller)
    {
    vtkDebugMacro("No controller, no viewport set.");
    return;
    }

  int rank = this->Controller->GetLocalProcessId();

  for (int y = 0; y < this->TileDimensions[1]; y++)
    {
    for (int x = 0; x < this->TileDimensions[0]; x++)
      {
      if (this->TileRanks[x][y] == rank)
        {
        // Transform camera for 3D actors.
        vtkPerspectiveTransform *transform = vtkPerspectiveTransform::New();
        transform->Identity();
        transform->Ortho(x*2.0/this->TileDimensions[0] - 1.0,
                         (x+1)*2.0/this->TileDimensions[0] - 1.0,
                         y*2.0/this->TileDimensions[1] - 1.0,
                         (y+1)*2.0/this->TileDimensions[1] - 1.0,
                         1.0, -1.0);
        this->SetTileViewportTransform(transform);
        transform->Delete();

        // Establish tiled window for 2D actors.
        if (this->RenderWindow)
          {
          // RenderWindow tiles from lower left instead of upper left.
          y = this->TileDimensions[1] - y - 1;
          this->RenderWindow->SetTileScale(this->TileDimensions);
          this->RenderWindow->SetTileViewport
            (x*(1.0/(float)(this->TileDimensions[0])), 
             y*(1.0/(float)(this->TileDimensions[1])), 
             (x+1.0)*(1.0/(float)(this->TileDimensions[0])), 
             (y+1.0)*(1.0/(float)(this->TileDimensions[1])));
          }

        return;
        }
      }
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetStrategy(StrategyType strategy)
{
  vtkDebugMacro("SetStrategy to " << strategy);

  if (this->Strategy == strategy) return;

  this->Strategy = strategy;
  this->StrategyDirty = 1;
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

void vtkIceTRenderManager::SetComposeOperation(ComposeOperationType operation)
{
  vtkDebugMacro("SetComposeOperation to " << operation);

  if (this->ComposeOperation == operation) return;

  this->ComposeOperation = operation;
  this->ComposeOperationDirty = 1;
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SetDataReplicationGroupColor(int color)
{
  // Just use ICE-T to figure out groups, since it can do that already.
  icetSetContext(this->Context->Handle);

  icetDataReplicationGroupColor(color);

  vtkIntArray *drg = vtkIntArray::New();
  drg->SetNumberOfComponents(1);
  GLint size;
  icetGetIntegerv(ICET_DATA_REPLICATION_GROUP_SIZE, &size);
  drg->SetNumberOfTuples(size);
  // Compiler, optimize away.
  if (sizeof(int) == sizeof(GLint))
    {
    icetGetIntegerv(ICET_DATA_REPLICATION_GROUP, (GLint *)drg->GetPointer(0));
    }
  else
    {
    GLint *tmparray = new GLint[size];
    icetGetIntegerv(ICET_DATA_REPLICATION_GROUP, tmparray);
    vtkstd::copy(tmparray, tmparray+size, drg->GetPointer(0));
    delete[] tmparray;
    }

  this->SetDataReplicationGroup(drg);
  drg->Delete();
}

//-----------------------------------------------------------------------------

double vtkIceTRenderManager::GetRenderTime()
{
  if (this->Controller)
    {
    double t;
    icetSetContext(this->Context->Handle);
    icetGetDoublev(ICET_RENDER_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}

//-----------------------------------------------------------------------------

double vtkIceTRenderManager::GetImageProcessingTime()
{
  return this->ImageProcessingTime + this->GetBufferReadTime()
    + this->GetCompositeTime();
}

//-----------------------------------------------------------------------------

double vtkIceTRenderManager::GetBufferReadTime()
{
  if (this->Controller)
    {
    double t;
    icetSetContext(this->Context->Handle);
    icetGetDoublev(ICET_BUFFER_READ_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}

//-----------------------------------------------------------------------------

double vtkIceTRenderManager::GetBufferWriteTime()
{
  if (this->Controller)
    {
    double t;
    icetSetContext(this->Context->Handle);
    icetGetDoublev(ICET_BUFFER_WRITE_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}

//-----------------------------------------------------------------------------

double vtkIceTRenderManager::GetCompositeTime()
{
  if (this->Controller)
    {
    double t;
    icetSetContext(this->Context->Handle);
    icetGetDoublev(ICET_COMPOSITE_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::StartRender()
{
  if (this->FullImageSharesData)
    {
    this->FullImage->Initialize();
    this->FullImageSharesData = 0;
    }

  if (this->ReducedImageSharesData)
    {
    this->ReducedImage->Initialize();
    this->ReducedImageSharesData = 0;
    }

  this->Superclass::StartRender();
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SatelliteStartRender()
{
  if (this->FullImageSharesData)
    {
    this->FullImage->Initialize();
    this->FullImageSharesData = 0;
    }

  if (this->ReducedImageSharesData)
    {
    this->ReducedImage->Initialize();
    this->ReducedImageSharesData = 0;
    }

  this->Superclass::SatelliteStartRender();
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::SendWindowInformation()
{
  vtkDebugMacro("Sending Window Information");

  this->Superclass::SendWindowInformation();

  struct IceTInformation info;
  info.TilesDirty = this->TilesDirty;
  info.Strategy = this->Strategy;
  info.ComposeOperation = this->ComposeOperation;

  int numProcs = this->Controller->GetNumberOfProcesses();
  for (int id = 0; id < numProcs; id++)
    {
    if (id == this->RootProcessId) continue;

    this->Controller->Send((int *)&info, ICET_INFO_SIZE, id,
                           vtkIceTRenderManager::ICET_INFO_TAG);
    if (this->TilesDirty)
      {
      this->Controller->Send(&this->TileDimensions[0], 1, id,
                             vtkIceTRenderManager::NUM_TILES_X_TAG);
      this->Controller->Send(&this->TileDimensions[1], 1, id,
                             vtkIceTRenderManager::NUM_TILES_Y_TAG);
      for (int x = 0; x < this->TileDimensions[0]; x++)
        {
        this->Controller->Send(this->TileRanks[x], this->TileDimensions[1], id,
                               vtkIceTRenderManager::TILE_RANKS_TAG);
        }
      }
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::ReceiveWindowInformation()
{
  vtkDebugMacro("Receiving Window Information");

  this->Superclass::ReceiveWindowInformation();

  struct IceTInformation info;
  this->Controller->Receive((int *)&info, ICET_INFO_SIZE, this->RootProcessId,
                            vtkIceTRenderManager::ICET_INFO_TAG);
  if (info.TilesDirty)
    {
    int NewNumTilesX, NewNumTilesY;
    this->Controller->Receive(&NewNumTilesX, 1, 0,
                              vtkIceTRenderManager::NUM_TILES_X_TAG);
    this->Controller->Receive(&NewNumTilesY, 1, 0,
                              vtkIceTRenderManager::NUM_TILES_Y_TAG);
    this->SetTileDimensions(NewNumTilesX, NewNumTilesY);
    for (int x = 0; x < this->TileDimensions[0]; x++)
      {
      this->Controller->Receive(this->TileRanks[x], this->TileDimensions[1], 0,
                                vtkIceTRenderManager::TILE_RANKS_TAG);
      }
    }

  this->SetStrategy((StrategyType)info.Strategy);
  this->SetComposeOperation((ComposeOperationType)info.ComposeOperation);
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::PreRenderProcessing()
{
  vtkDebugMacro("PreRenderProcessing");
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkRendererCollection *rens = renWin->GetRenderers();
  vtkRenderer* ren;

  // Normally if this->UseCompositing was false, we would return here.  But
  // if we did that, the tile display would become invalid.  Instead, in
  // UpdateIceTContext we tell ICE-T that the data is replicated on all
  // nodes.  This will make ICE-T turn off compositing and make each
  // process render its local tile (if any).  The only real overhead is an
  // unnecessary frame buffer read back by ICE-T.  If that's a problem, we
  // could always add a special case to ICE-T...

  this->UpdateIceTContext();

  // Only composite the first renderer.
  rens->InitTraversal();
  ren = rens->GetNextItem();
  if (ren == NULL)
    {
    vtkErrorMacro("Missing renderer.");
    return;
    }
  vtkIceTRenderer *icetRen = vtkIceTRenderer::SafeDownCast(ren);
  if (icetRen == NULL)
    {
    vtkWarningMacro("vtkIceTRenderManager used with renderer that is not "
                    "vtkIceTRenderer.\n"
                    "Remember to use\n\n"
                    "    vtkParallelRenderManager::MakeRenderer()\n\n"
                    "in place of vtkRenderer::New()");
    }
  else
    {
    icetRen->SetComposeNextFrame(1);
    if (this->SortingKdTree)
      {
      // Setup ICE-T context for correct sorting.
      icetEnable(ICET_ORDERED_COMPOSITE);
      vtkIntArray *orderedProcessIds = vtkIntArray::New();

      // Order all the regions.
      this->SortingKdTree->DepthOrderAllProcesses(
                         icetRen->GetActiveCamera()->GetDirectionOfProjection(),
                         orderedProcessIds);
      // Compiler, optimize away.
      if (sizeof(int) == sizeof(GLint))
        {
        icetCompositeOrder((GLint *)orderedProcessIds->GetPointer(0));
        }
      else
        {
        vtkIdType numprocs = orderedProcessIds->GetNumberOfTuples();
        GLint *tmparray = new GLint[numprocs];
        const int *opiarray = orderedProcessIds->GetPointer(0);
        vtkstd::copy(opiarray, opiarray+numprocs, tmparray);
        delete[] tmparray;
        }
      orderedProcessIds->Delete();
      }
    else
      {
      icetDisable(ICET_ORDERED_COMPOSITE);
      }
    }

  if (this->ImageReductionFactor > 1)
    {
    // Restore viewports.  ICE-T will handle reduced images better on its own.
    double *viewport = ren->GetViewport();
    ren->SetViewport(viewport[0]*this->ImageReductionFactor,
                     viewport[1]*this->ImageReductionFactor,
                     viewport[2]*this->ImageReductionFactor,
                     viewport[3]*this->ImageReductionFactor);
    }

  // For all subsequent renderers, assume that the data is replicated, do
  // no compositing, and asjust the camera to focus on the displayed tile.
  for (ren = rens->GetNextItem(); ren != NULL; ren = rens->GetNextItem())
    {
    ren->GetActiveCamera()->SetUserTransform(this->GetTileViewportTransform());
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::PostRenderProcessing()
{
  vtkDebugMacro("PostRenderProcessing");

  this->Controller->Barrier();

  if (this->WriteBackImages)
    {
    this->RenderWindowImageUpToDate = true;
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::ReadReducedImage()
{
  vtkDebugMacro("Reading Reduced Image");

  if (this->ReducedImageUpToDate)
    {
    return;
    }

  GLboolean icet_color_buffer_valid;
  icetGetBooleanv(ICET_COLOR_BUFFER_VALID, &icet_color_buffer_valid);
  if (!icet_color_buffer_valid)
    {
    this->Superclass::ReadReducedImage();
    return;
    }

  vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  vtkRenderer *ren = rens->GetNextItem();
  vtkIceTRenderer *icetren = vtkIceTRenderer::SafeDownCast(ren);
  if (!icetren)
    {
    this->Superclass::ReadReducedImage();
    return;
    }

#ifndef GL_BGRA
#define GL_BGRA GL_BGRA_EXT
#endif
  GLint color_format;
  icetGetIntegerv(ICET_COLOR_FORMAT, &color_format);
  if (color_format == GL_RGBA)
    {
    this->ReducedImage->SetArray(icetGetColorBuffer(),
                                 this->ReducedImageSize[0]
                                 *this->ReducedImageSize[1]*4,
                                 1);
    this->ReducedImage->SetNumberOfComponents(4);
    this->ReducedImage->SetNumberOfTuples(  this->ReducedImageSize[0]
                                          * this->ReducedImageSize[1]);
    this->ReducedImageSharesData = 1;
    }
  else if (color_format == GL_BGRA)
    {
    this->ReducedImage->SetNumberOfComponents(4);
    this->ReducedImage->SetNumberOfTuples(  this->ReducedImageSize[0]
                                          * this->ReducedImageSize[1]);
    unsigned char *dest
      = this->ReducedImage->WritePointer(0,
                                         4*this->ReducedImageSize[0]
                                         *this->ReducedImageSize[1]);
    unsigned char *src = icetGetColorBuffer();
    int image_size = this->ReducedImageSize[0]*this->ReducedImageSize[1];
    for (int i = 0; i < image_size; i++, dest += 4, src += 4)
      {
      dest[0] = src[2];
      dest[1] = src[1];
      dest[2] = src[0];
      dest[3] = src[3];
      }
    }
  else
    {
    vtkErrorMacro("ICE-T using unknown image format.");
    this->Superclass::ReadReducedImage();
    return;
    }

  this->ReducedImageUpToDate = true;

  if (this->ImageReductionFactor == 1)
    {
    this->FullImage->SetArray(this->ReducedImage->GetPointer(0),
                              this->FullImageSize[0]*this->FullImageSize[1]*4,
                              1);
    this->FullImage->SetNumberOfComponents(4);
    this->FullImage->SetNumberOfTuples(  this->FullImageSize[0]
                                       * this->FullImageSize[1]);
    this->FullImageSharesData = 1;
    this->FullImageUpToDate = true;
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderManager::PrintSelf(ostream &os, vtkIndent indent)
{
  int x, y;

  this->Superclass::PrintSelf(os, indent);

  os << indent << "ICE-T Context: " << this->Context->Handle << endl;

  os << indent << "Display: " << this->TileDimensions[0]
     << " X " << this->TileDimensions[1] << " with display ranks" << endl;
  for (y = 0; y < this->TileDimensions[1]; y++)
    {
    os << indent << "    ";
    for (x = 0; x < this->TileDimensions[0]; x++)
      {
      os.width(4);
      os << this->GetTileRank(x, y);
      }
    os << endl;
    }
  os.width(0);

  os << indent << "Strategy: ";
  switch (this->Strategy)
    {
    case DEFAULT: os << "DEFAULT"; break;
    case REDUCE:  os << "REDUCE";  break;
    case VTREE:   os << "VTREE";   break;
    case SPLIT:   os << "SPLIT";   break;
    case SERIAL:  os << "SERIAL";  break;
    case DIRECT:  os << "DIRECT";  break;
    }
  os << endl;

  os << indent << "Compose Operation: ";
  switch (this->ComposeOperation)
    {
    case CLOSEST: os << "closest to camera"; break;
    case OVER:    os << "Porter and Duff OVER operator"; break;
    }
  os << endl;

  vtkIndent i2 = indent.GetNextIndent();
  os << indent << "Sorting Kd tree: ";
  if (this->SortingKdTree)
    {
    os << endl;
    this->SortingKdTree->PrintSelf(os, i2);
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "Data Replication Group: ";
  if (this->DataReplicationGroup)
    {
    os << endl;
    this->DataReplicationGroup->PrintSelf(os, i2);
    }
  else
    {
    os << "(none)" << endl;
    }
}
