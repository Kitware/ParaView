/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTComposite.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkIceTComposite.h"

#include "vtkIceTRenderer.h"
#include "vtkIceTFactory.h"
#include <vtkObjectFactory.h>
#include <vtkCallbackCommand.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkMPIController.h>
#include <vtkMPI.h>

#include <string.h>
#include <GL/ice-t.h>


//******************************************************************
// Prototypes
//******************************************************************

static void SafeDestroyContext(IceTContext context);
//static void StartRender(vtkObject *caller, unsigned long event,
//      void *clientData, void *);

//******************************************************************
// vtkIceTComposite implementation.
//******************************************************************

void vtkIceTComposite::InstallFactory()
{
  vtkObjectFactory::RegisterFactory(vtkIceTFactory::New());
}

vtkStandardNewMacro(vtkIceTComposite);

vtkIceTComposite::vtkIceTComposite()
{
  vtkWarningMacro("vtkIceTComposite deprecated.  Use vtkIceTRenderManager.");
  if (!icetInitialized())
    {
    icetInit(MPI_COMM_WORLD);
    }

  this->Context = ICET_DEFAULT_CONTEXT;
  this->SetController(this->Controller);
  this->ContextDirty = 1;

  this->NumTilesX = 1;
  this->NumTilesY = 1;
  this->TileRanks = new int*[1];
  this->TileRanks[0] = new int[1];
  this->TileRanks[0][0] = 0;
  this->TilesDirty = 1;

  this->Strategy = DEFAULT;
  this->StrategyDirty = 1;

  this->lastKnownReductionFactor = 0;
}

vtkIceTComposite::~vtkIceTComposite()
{
  SafeDestroyContext(this->Context);
  for (int x = 0; x < this->NumTilesX; x++)
    {
    delete[] this->TileRanks[x];
    }
  delete[] this->TileRanks;

  this->SetRenderWindow(NULL);
}

// Call the super SetRenderWindow.  Also make sure StartRender is called
// on every processor, not just processor 0.
void vtkIceTComposite::SetRenderWindow(vtkRenderWindow *renWin)
{
  vtkDebugMacro("SetRenderWindow to " << renWin);

  if (   this->RenderWindow
      && (!this->Controller || this->Controller->GetLocalProcessId() != 0) )
    {
    this->RenderWindow->RemoveObserver(this->StartTag);
    }

  this->vtkCompositeManager::SetRenderWindow(renWin);

  if (renWin != NULL)
    {
    // ICE-T requires double buffering.
    renWin->DoubleBufferOn();
    }
}

void vtkIceTComposite::SetController(vtkMultiProcessController *controller)
{
  vtkDebugMacro("SetController to " << controller);

  this->vtkCompositeManager::SetController(controller);

  SafeDestroyContext(this->Context);

  vtkCommunicator *communicator = this->Controller->GetCommunicator();
  if (!communicator->IsA("vtkMPICommunicator"))
    {
    vtkErrorMacro("vtkIceTComposite parallel compositor currently works only with an MPI communicator.");
    this->Context = icetCreateContext(MPI_COMM_WORLD);
    }
  else
    {
    vtkMPICommunicator *MPICommunicator;
    MPICommunicator = vtkMPICommunicator::SafeDownCast(communicator);
    MPI_Comm handle = *MPICommunicator->GetMPIComm()->GetHandle();
    this->Context = icetCreateContext(handle);
    vtkDebugMacro("Created new ICE-T context.");
    }

  this->ContextDirty = 1;
}

void vtkIceTComposite::UpdateIceTContext(int screenWidth, int screenHeight)
{
  vtkDebugMacro("UpdateIceTContext");

  if (this->ReductionFactor != this->lastKnownReductionFactor)
    {
    this->lastKnownReductionFactor = this->ReductionFactor;
    this->TilesDirty = 1;
    }


  this->ContextDirty = 1;
}

void vtkIceTComposite::UpdateIceTContext(int screenWidth, int screenHeight)
{
  vtkDebugMacro("UpdateIceTContext");

  if (this->ReductionFactor != this->lastKnownReductionFactor)
    {
    this->lastKnownReductionFactor = this->ReductionFactor;
    this->TilesDirty = 1;
    }

  icetSetContext(this->Context);

  if (   this->ContextDirty || this->TilesDirty
      || (this->CleanScreenWidth != screenWidth)
      || (this->CleanScreenHeight != screenHeight) )
    {
    int x, y;
    int reducedScreenWidth, reducedScreenHeight;

    if (this->ReductionFactor > 1)
      {
      reducedScreenWidth = screenWidth/this->ReductionFactor;
      reducedScreenHeight = screenHeight/this->ReductionFactor;
      }
    else
      {
      reducedScreenWidth = screenWidth;
      reducedScreenHeight = screenHeight;
      }

    icetResetTiles();
    for (x = 0; x < this->NumTilesX; x++)
      {
      for (y = 0; y < this->NumTilesY; y++)
  {
  icetAddTile(x*reducedScreenWidth,
        (this->NumTilesY-y-1)*reducedScreenHeight,
        reducedScreenWidth, reducedScreenHeight,
        this->TileRanks[x][y]);
  }
      }
    icetRenderableSize(screenWidth, screenHeight);
    this->TilesDirty = 0;
    this->CleanScreenWidth = screenWidth;
    this->CleanScreenHeight = screenHeight;
    }

  if (this->ContextDirty || this->StrategyDirty)
    {
    switch (this->Strategy)
      {
      case DEFAULT: icetStrategy(ICET_STRATEGY_DEFAULT); break;
      case REDUCE:  icetStrategy(ICET_STRATEGY_REDUCE);  break;
      case VTREE:   icetStrategy(ICET_STRATEGY_VTREE);   break;
      case SPLIT:   icetStrategy(ICET_STRATEGY_SPLIT);   break;
      case SERIAL:  icetStrategy(ICET_STRATEGY_SERIAL);  break;
      case DIRECT:  icetStrategy(ICET_STRATEGY_DIRECT);  break;
      }
    this->StrategyDirty = 0;
    }

  if (this->ContextDirty)
    {
    if (this->Debug)
      {
      icetDiagnostics(ICET_DIAG_DEBUG | ICET_DIAG_ALL_NODES);
      }
    else
      {
      icetDiagnostics(ICET_DIAG_WARNINGS | ICET_DIAG_ALL_NODES);
      }
    icetEnable(ICET_DISPLAY);
    icetEnable(ICET_DISPLAY_COLORED_BACKGROUND);
    icetEnable(ICET_DISPLAY_INFLATE);
    }

  this->ContextDirty = 0;
}

void vtkIceTComposite::SetNumTilesX(int tilesX)
{
  vtkDebugMacro("SetNumTilesX " << tilesX);

  this->ChangeTileDims(tilesX, this->NumTilesY);
}
void vtkIceTComposite::SetNumTilesY(int tilesY)
{
  vtkDebugMacro("SetNumTilesY " << tilesY);

  this->ChangeTileDims(this->NumTilesX, tilesY);
}

void vtkIceTComposite::ChangeTileDims(int tilesX, int tilesY)
{
  vtkDebugMacro("ChangeTileDims " << tilesX << " " << tilesY);

  int x, y;
  int **NewTileRanks;

  NewTileRanks = new int*[tilesX];
  for (x = 0; x < tilesX; x++)
    {
    NewTileRanks[x] = new int[tilesY];
    for (y = 0; y < tilesY; y++)
      {
      if ( (y < this->NumTilesY) && (x < this->NumTilesX))
  {
  NewTileRanks[x][y] = this->TileRanks[x][y];
  }
      else
  {
  NewTileRanks[x][y] = y*tilesX + x;
  }
      }
    delete[] this->TileRanks[x];
    }

  delete[] this->TileRanks;
  this->TileRanks = NewTileRanks;
  this->NumTilesX = tilesX;
  this->NumTilesY = tilesY;
  this->TilesDirty = 1;
}

int vtkIceTComposite::GetTileRank(int x, int y)
{
  vtkDebugMacro("GetTileRank " << x << " " << y);

  if (   (x < 0) || (x >= this->NumTilesX)
      || (y < 0) || (y >= this->NumTilesY) )
    {
    vtkErrorMacro("Invalid tile " << x << ", " << y);
    return -1;
    }

  return this->TileRanks[x][y];
}
void vtkIceTComposite::SetTileRank(int x, int y, int rank)
{
  vtkDebugMacro("SetTileRank " << x << " " << y << " " << rank);

  if (   (x < 0) || (x >= this->NumTilesX)
      || (y < 0) || (y >= this->NumTilesY) )
    {
    vtkErrorMacro("Invalid tile " << x << ", " << y);
    return;
    }

  this->TileRanks[x][y] = rank;
  this->TilesDirty = 1;
}

void vtkIceTComposite::SetStrategy(StrategyType strategy)
{
  vtkDebugMacro("SetStrategy to " << strategy);

  if (this->Strategy == strategy) return;

  this->Strategy = strategy;
  this->StrategyDirty = 1;
}

void vtkIceTComposite::SetStrategy(const char *strategy)
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

double vtkIceTComposite::GetGetBuffersTime()
{
  double t;
  icetGetDoublev(ICET_BUFFER_READ_TIME, &t);
  return t;
}
double vtkIceTComposite::GetSetBuffersTime()
{
  double t;
  icetGetDoublev(ICET_BUFFER_WRITE_TIME, &t);
  return t;
}
double vtkIceTComposite::GetCompositeTime()
{
  double t;
  icetGetDoublev(ICET_COMPOSITE_TIME, &t);
  return t;
}
double vtkIceTComposite::GetMaxRenderTime()
{
  double t;
  icetGetDoublev(ICET_RENDER_TIME, &t);
  return t;
}

//We always tell the superclass to abort the composite, because it
//is already done by that time.  Since this method is called after
//render, it is also a good place to do the finishing touches that
//we still need but vtkCompositeManager will abort.
int vtkIceTComposite::CheckForAbortComposite()
{
  this->Controller->Barrier();

  this->RenderWindow->SwapBuffersOn();
  this->RenderWindow->Frame();

  return 1;
  //return 0;
}

void vtkIceTComposite::PrepareForCompositeRender()
{
  int *size = this->RenderWindow->GetSize();
  // Make sure size is valid.
  if ((size[0] == 0) || (size[1] == 0))
    {
    this->RenderWindow->SetSize(300, 300);
    size = this->RenderWindow->GetSize();
    }
  this->UpdateIceTContext(size[0], size[1]);

  vtkRendererCollection *rcol = this->RenderWindow->GetRenderers();
  rcol->InitTraversal();
  vtkRenderer *ren;
  for (ren = rcol->GetNextItem(); ren != NULL; ren = rcol->GetNextItem())
    {
    vtkIceTRenderer *icetRen = vtkIceTRenderer::SafeDownCast(ren);
    if (icetRen == NULL)
      {
      vtkWarningMacro("vtkIceTComposite used with renderer that is not "
          "vtkIceTRenderer.\n"
          "Try adding\n\n"
          "    vtkIceTComposite::InstallFactory()\n\n"
          "to the beginning of your program, or explictly use\n\n"
          "    vtkIceTRenderer::New()\n\n"
          "in place of vtkRenderer::New()");
      }
    else
      {
      icetRen->SetComposeNextFrame(1);
      }
    }
}

void vtkIceTComposite::StartRender()
{
  vtkDebugMacro("vtkIceTComposite::StartRender");

  if (this->UseCompositing)
    {
    this->PrepareForCompositeRender();

    int numProcs = this->Controller->GetNumberOfProcesses();
    for (int id = 1; id < numProcs; id++)
      {
      this->Controller->Send(&this->ReductionFactor, 1, id,
           vtkIceTComposite::REDUCTION_FACTOR_TAG);
      this->Controller->Send(&this->TilesDirty, 1, id,
           vtkIceTComposite::TILES_DIRTY_TAG);
      if (this->TilesDirty)
  {
  this->Controller->Send(&this->NumTilesX, 1, id,
             vtkIceTComposite::NUM_TILES_X_TAG);
  this->Controller->Send(&this->NumTilesY, 1, id,
             vtkIceTComposite::NUM_TILES_Y_TAG);
  for (int x = 0; x < this->NumTilesX; x++)
    {
    this->Controller->Send(this->TileRanks[x], this->NumTilesY, id,
         vtkIceTComposite::TILE_RANKS_TAG);
    }
  }
      this->Controller->Send((int *)&this->Strategy, 1, id,
           vtkIceTComposite::STRATEGY_TAG);
      }

    int saveReductionFactor = this->ReductionFactor;
    this->ReductionFactor = 1;
    this->vtkCompositeManager::StartRender();
    this->ReductionFactor = saveReductionFactor;

    // Manually swap buffers on all processes.
    this->RenderWindow->SwapBuffersOff();
    }
}

void vtkIceTComposite::SatelliteStartRender()
{
  vtkDebugMacro("vtkIceTComposite::SatelliteStartRender");

  if (this->UseCompositing)
    {
    // Get ICE-T specific parameters.
    int NewReductionFactor;
    this->Controller->Receive(&NewReductionFactor, 1, 0,
            vtkIceTComposite::REDUCTION_FACTOR_TAG);
    this->SetReductionFactor(NewReductionFactor);

    int NewTilesDirty;
    this->Controller->Receive(&NewTilesDirty, 1, 0,
            vtkIceTComposite::TILES_DIRTY_TAG);
    if (NewTilesDirty)
      {
      int NewNumTilesX, NewNumTilesY;
      this->Controller->Receive(&NewNumTilesX, 1, 0,
        vtkIceTComposite::NUM_TILES_X_TAG);
      this->Controller->Receive(&NewNumTilesY, 1, 0,
        vtkIceTComposite::NUM_TILES_X_TAG);
      this->ChangeTileDims(NewNumTilesX, NewNumTilesY);
      for (int x = 0; x < this->NumTilesX; x++)
  {
  this->Controller->Receive(this->TileRanks[x], this->NumTilesY, 0,
          vtkIceTComposite::TILE_RANKS_TAG);
  }
      }

    int NewStrategy;
    this->Controller->Receive(&NewStrategy, 1, 0,
            vtkIceTComposite::STRATEGY_TAG);
    this->SetStrategy((StrategyType)NewStrategy);

    this->vtkCompositeManager::SatelliteStartRender();

    this->PrepareForCompositeRender();

    // Manually swap buffers on all processes.
    this->RenderWindow->SwapBuffersOff();
    }
}

void vtkIceTComposite::SetReductionFactorRMI()
{
  vtkDebugMacro("SetReductionFactorRMI");

  int NewReductionFactor;
  this->Controller->Receive(&NewReductionFactor, 1, 0, REDUCTION_FACTOR_TAG);

  this->SetReductionFactor(NewReductionFactor);
}

void vtkIceTComposite::InitializeRMIs()
{
  vtkDebugMacro("vtkIceTComposite::InitializeRMIs");

  this->vtkCompositeManager::InitializeRMIs();

  if (this->Controller == NULL)
    {
    return;
    }
}

//This should never be called.
void vtkIceTComposite::CompositeBuffer(int width, int height, int useCharFlag,
               vtkDataArray *pBuf, vtkFloatArray *zBuf,
               vtkDataArray *pTmp, vtkFloatArray *zTmp)
{
  (void)width;
  (void)height;
  (void)useCharFlag;
  (void)pBuf;
  (void)zBuf;
  (void)pTmp;
  (void)zTmp;
  vtkErrorMacro("CompositeBuffer called");
}


void vtkIceTComposite::PrintSelf(ostream &os, vtkIndent indent)
{
  int x, y;

  this->vtkCompositeManager::PrintSelf(os, indent);

  os << indent << "ICE-T Context: " << this->Context << endl;

  os << indent << "Display: " << this->NumTilesX
     << " X " << this->NumTilesY << " with display ranks" << endl;
  for (y = 0; y < this->NumTilesY; y++)
    {
    os << indent << "    ";
    for (x = 0; x < this->NumTilesX; x++)
      {
      os.width(4);
  if (context != ICET_DEFAULT_CONTEXT)
    {
    if (context == icetGetContext())
      {
      icetSetContext(ICET_DEFAULT_CONTEXT);
      }
    icetDestroyContext(context);
    }
  os << indent << "Strategy: " << this->Strategy << endl;
}
