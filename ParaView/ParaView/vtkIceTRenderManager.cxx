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
#include "vtkIceTRenderer.h"
#include "vtkPKdTree.h"
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkMPIController.h>
#include <vtkMPI.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkUnsignedCharArray.h>
#include <GL/ice-t_mpi.h>
#include "vtkCamera.h"

//******************************************************************
// Hidden structures.
//******************************************************************
struct IceTInformation {
  int TilesDirty;
  int Strategy;
  int ComposeOperation;
};
const int ICET_INFO_SIZE = sizeof(struct IceTInformation)/sizeof(int);

//******************************************************************
// vtkIceTRenderManager implementation.
//******************************************************************

vtkCxxRevisionMacro(vtkIceTRenderManager, "1.7");
vtkStandardNewMacro(vtkIceTRenderManager);

vtkCxxSetObjectMacro(vtkIceTRenderManager, SortingKdTree, vtkPKdTree);

vtkIceTRenderManager::vtkIceTRenderManager()
{
  this->NumTilesX = 1;
  this->NumTilesY = 1;
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
}

vtkIceTRenderManager::~vtkIceTRenderManager()
{
  this->SetController(NULL);
  for (int x = 0; x < this->NumTilesX; x++)
    {
    delete[] this->TileRanks[x];
    }
  delete[] this->TileRanks;
  this->SetSortingKdTree(NULL);
}

vtkRenderer *vtkIceTRenderManager::MakeRenderer()
{
  return vtkIceTRenderer::New();
}

void vtkIceTRenderManager::SetController(vtkMultiProcessController *controller)
{
  vtkDebugMacro("SetController to " << controller);

  if (controller == this->Controller)
    {
    return;
    }

  vtkCommunicator *communicator;
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
    icetDestroyContext(this->Context);
    }

  this->Superclass::SetController(controller);

  if (this->Controller)
    {
    vtkMPICommunicator *MPICommunicator;
    MPICommunicator = vtkMPICommunicator::SafeDownCast(communicator);
    MPI_Comm handle = *MPICommunicator->GetMPIComm()->GetHandle();
    IceTCommunicator icet_comm = icetCreateMPICommunicator(handle);
    this->Context = icetCreateContext(icet_comm);
    icetDestroyMPICommunicator(icet_comm);
    vtkDebugMacro("Created new ICE-T context.");
    }

  this->ContextDirty = 1;
}

void vtkIceTRenderManager::UpdateIceTContext()
{
  vtkDebugMacro("UpdateIceTContext");

  icetSetContext(this->Context);

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
    for (x = 0; x < this->NumTilesX; x++)
      {
      for (y = 0; y < this->NumTilesY; y++)
        {
        icetAddTile(x*this->ReducedImageSize[0],
                    (this->NumTilesY-y-1)*this->ReducedImageSize[1],
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
    }

  this->ContextUpdateTime.Modified();
  this->ContextDirty = 0;
}

void vtkIceTRenderManager::SetNumTilesX(int tilesX)
{
  vtkDebugMacro("SetNumTilesX " << tilesX);

  this->ChangeTileDims(tilesX, this->NumTilesY);
}
void vtkIceTRenderManager::SetNumTilesY(int tilesY)
{
  vtkDebugMacro("SetNumTilesY " << tilesY);

  this->ChangeTileDims(this->NumTilesX, tilesY);
}

void vtkIceTRenderManager::ChangeTileDims(int tilesX, int tilesY)
{
  vtkDebugMacro("ChangeTileDims " << tilesX << " " << tilesY);

  if ((this->NumTilesX == tilesX) && (this->NumTilesY == tilesY))
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
      if ( (y < this->NumTilesY) && (x < this->NumTilesX))
        {
        NewTileRanks[x][y] = this->TileRanks[x][y];
        }
      else
        {
        NewTileRanks[x][y] = y*tilesX + x;
        }
      }
    if (x < this->NumTilesX)
      {
      delete[] this->TileRanks[x];
      }
    }

  delete[] this->TileRanks;
  this->TileRanks = NewTileRanks;
  this->NumTilesX = tilesX;
  this->NumTilesY = tilesY;
  this->TilesDirty = 1;
}

int vtkIceTRenderManager::GetTileRank(int x, int y)
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
void vtkIceTRenderManager::SetTileRank(int x, int y, int rank)
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

void vtkIceTRenderManager::SetStrategy(StrategyType strategy)
{
  vtkDebugMacro("SetStrategy to " << strategy);

  if (this->Strategy == strategy) return;

  this->Strategy = strategy;
  this->StrategyDirty = 1;
}

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

void vtkIceTRenderManager::SetComposeOperation(ComposeOperationType operation)
{
  vtkDebugMacro("SetComposeOperation to " << operation);

  if (this->ComposeOperation == operation) return;

  this->ComposeOperation = operation;
  this->ComposeOperationDirty = 1;
}

double vtkIceTRenderManager::GetRenderTime()
{
  if (this->Controller)
    {
    double t;
    icetSetContext(this->Context);
    icetGetDoublev(ICET_RENDER_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}
double vtkIceTRenderManager::GetImageProcessingTime()
{
  return this->ImageProcessingTime + this->GetBufferReadTime()
    + this->GetCompositeTime();
}
double vtkIceTRenderManager::GetBufferReadTime()
{
  if (this->Controller)
    {
    double t;
    icetSetContext(this->Context);
    icetGetDoublev(ICET_BUFFER_READ_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}
double vtkIceTRenderManager::GetBufferWriteTime()
{
  if (this->Controller)
    {
    double t;
    icetSetContext(this->Context);
    icetGetDoublev(ICET_BUFFER_WRITE_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}
double vtkIceTRenderManager::GetCompositeTime()
{
  if (this->Controller)
    {
    double t;
    icetSetContext(this->Context);
    icetGetDoublev(ICET_COMPOSITE_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}

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
      this->Controller->Send(&this->NumTilesX, 1, id,
                             vtkIceTRenderManager::NUM_TILES_X_TAG);
      this->Controller->Send(&this->NumTilesY, 1, id,
                             vtkIceTRenderManager::NUM_TILES_Y_TAG);
      for (int x = 0; x < this->NumTilesX; x++)
        {
        this->Controller->Send(this->TileRanks[x], this->NumTilesY, id,
                               vtkIceTRenderManager::TILE_RANKS_TAG);
        }
      }
    }
}

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
    this->ChangeTileDims(NewNumTilesX, NewNumTilesY);
    for (int x = 0; x < this->NumTilesX; x++)
      {
      this->Controller->Receive(this->TileRanks[x], this->NumTilesY, 0,
                                vtkIceTRenderManager::TILE_RANKS_TAG);
      }
    }

  this->SetStrategy((StrategyType)info.Strategy);
  this->SetComposeOperation((ComposeOperationType)info.ComposeOperation);
}

void vtkIceTRenderManager::PreRenderProcessing()
{
  vtkDebugMacro("PreRenderProcessing");

  // Code taken from my tile display module.
  if (!this->UseCompositing)
    {
    vtkCamera* cam;
    vtkRenderWindow* renWin = this->RenderWindow;
    vtkRendererCollection *rens;
    vtkRenderer* ren;

    rens = renWin->GetRenderers();
    rens->InitTraversal();
    ren = rens->GetNextItem();
    if (ren)
      {
      cam = ren->GetActiveCamera();
      }

    int x, y;
    int tileIdx = this->Controller->GetLocalProcessId();
    y = tileIdx/this->NumTilesX;
    x = tileIdx - y*this->NumTilesX;
    // Flip the y axis to match IceT
    y = this->NumTilesY-1-y;
    // Setup the camera for this tile.
    cam->SetWindowCenter(1.0-(double)(this->NumTilesX) + 2.0*(double)x,
                         1.0-(double)(this->NumTilesY) + 2.0*(double)y);


    return;
    }


  this->UpdateIceTContext();

  vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
  vtkRenderer *ren;
  int i;
  for (rens->InitTraversal(), i = 0; (ren = rens->GetNextItem()); i++)
    {
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
        this->SortingKdTree->DepthOrderAllProcesses(icetRen->GetActiveCamera(),
                                                    orderedProcessIds);
        icetCompositeOrder(orderedProcessIds->GetPointer(0));
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
    }

  this->RenderWindow->SwapBuffersOff();
}

void vtkIceTRenderManager::PostRenderProcessing()
{
  vtkDebugMacro("PostRenderProcessing");

  this->RenderWindow->SwapBuffersOn();
  this->Controller->Barrier();

  this->RenderWindow->Frame();

  if (this->WriteBackImages)
    {
    this->RenderWindowImageUpToDate = true;
    }
}

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

void vtkIceTRenderManager::PrintSelf(ostream &os, vtkIndent indent)
{
  int x, y;

  this->Superclass::PrintSelf(os, indent);

  os << indent << "ICE-T Context: " << this->Context << endl;

  os << indent << "Display: " << this->NumTilesX
     << " X " << this->NumTilesY << " with display ranks" << endl;
  for (y = 0; y < this->NumTilesY; y++)
    {
    os << indent << "    ";
    for (x = 0; x < this->NumTilesX; x++)
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
}


//******************************************************************
//Local function/method implementation.
//******************************************************************

