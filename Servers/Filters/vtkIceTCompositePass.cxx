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
#include "vtkIceTCompositePass.h"

#include "vtkCamera.h"
#include "vtkFrameBufferObject.h"
#include "vtkIceTContext.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkRenderer.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTilesHelper.h"

#include <assert.h>
#include "vtkgl.h"
#include <GL/ice-t.h>

namespace
{
  static vtkIceTCompositePass* IceTDrawCallbackHandle = NULL;
  static const vtkRenderState* IceTDrawCallbackState = NULL;
  void IceTDrawCallback()
    {
    if (IceTDrawCallbackState && IceTDrawCallbackHandle)
      {
      IceTDrawCallbackHandle->Draw(IceTDrawCallbackState);
      }
    }
};

vtkStandardNewMacro(vtkIceTCompositePass);
vtkCxxRevisionMacro(vtkIceTCompositePass, "$Revision$");
vtkCxxSetObjectMacro(vtkIceTCompositePass, RenderPass, vtkRenderPass);
vtkCxxSetObjectMacro(vtkIceTCompositePass, KdTree, vtkPKdTree);
vtkCxxSetObjectMacro(vtkIceTCompositePass, Controller,
  vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkIceTCompositePass::vtkIceTCompositePass()
{
  this->IceTContext = vtkIceTContext::New();
  this->Controller = 0;
  this->RenderPass = 0;
  this->KdTree = 0;
  this->TileMullions[0] = this->TileMullions[1] = 0;
  this->TileDimensions[0] = 1;
  this->TileDimensions[1] = 1;

  this->LastTileDimensions[0] = this->LastTileDimensions[1] = -1;
  this->LastTileMullions[0] = this->LastTileMullions[1] = -1;
  this->LastTileViewport[0] = this->LastTileViewport[1] =
    this->LastTileViewport[2] = this->LastTileViewport[3] = 0;

  this->DataReplicatedOnAllProcesses = false;
  this->ImageReductionFactor = 1;

  this->UseOrderedCompositing = false;
}

//----------------------------------------------------------------------------
vtkIceTCompositePass::~vtkIceTCompositePass()
{
  this->SetKdTree(0);
  this->SetRenderPass(0);
  this->SetController(0);
  this->IceTContext->Delete();
  this->IceTContext = 0;
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::ReleaseGraphicsResources(vtkWindow* window)
{
  this->Superclass::ReleaseGraphicsResources(window);
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::SetupContext(const vtkRenderState* render_state)
{
  this->IceTContext->SetController(this->Controller);
  if (!this->IceTContext->IsValid())
    {
    vtkErrorMacro("Could not initialize IceT context.");
    return;
    }

  this->IceTContext->MakeCurrent();
  //icetDiagnostics(ICET_DIAG_DEBUG | ICET_DIAG_ALL_NODES);

  // Irrespective of whether we are rendering in tile/display mode or not, we
  // need to pass appropriate tile parameters to IceT.
  this->UpdateTileInformation(render_state);

  // Set IceT compositing strategy.
  icetStrategy(ICET_STRATEGY_REDUCE);

  bool use_ordered_compositing =
    (this->KdTree && this->UseOrderedCompositing &&
     this->KdTree->GetNumberOfRegions() >=
     this->IceTContext->GetController()->GetNumberOfProcesses());

  // If translucent geometry is present, then we should not include
  // ICET_DEPTH_BUFFER_BIT in  the input-buffer argument.
  if (use_ordered_compositing)
    {
    icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT, ICET_COLOR_BUFFER_BIT);
    }
  else
    {
    icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT | ICET_DEPTH_BUFFER_BIT,
      ICET_COLOR_BUFFER_BIT | ICET_DEPTH_BUFFER_BIT);
    }

  icetDisable(ICET_FLOATING_VIEWPORT);
  if (use_ordered_compositing)
    {
    // if ordered compositing is enabled, pass the process order from the kdtree
    // to icet.

    // Setup ICE-T context for correct sorting.
    icetEnable(ICET_ORDERED_COMPOSITE);

    // Order all the regions.
    vtkIntArray *orderedProcessIds = vtkIntArray::New();
    vtkCamera *camera = render_state->GetRenderer()->GetActiveCamera();
    if (camera->GetParallelProjection())
      {
      this->KdTree->ViewOrderAllProcessesInDirection(
        camera->GetDirectionOfProjection(),
        orderedProcessIds);
      }
    else
      {
      this->KdTree->ViewOrderAllProcessesFromPosition(
        camera->GetPosition(), orderedProcessIds);
      }

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
      icetCompositeOrder(tmparray);
      delete[] tmparray;
      }
    orderedProcessIds->Delete();
    }
  else
    {
    icetDisable(ICET_ORDERED_COMPOSITE);
    }

  // Let IceT know the data bounds. This allows IceT to make smarter compositing
  // decisions.
  double allBounds[6];
  render_state->GetRenderer()->ComputeVisiblePropBounds(allBounds);
  //Try to detect when bounds are empty and try to let ICE-T know that
  //nothing is in bounds.
  if (allBounds[0] > allBounds[1])
    {
    cout << "nothing visible" << endl;
    float tmp = VTK_LARGE_FLOAT;
    icetBoundingVertices(1, ICET_FLOAT, 0, 1, &tmp);
    }
  else
    {
    icetBoundingBoxd(allBounds[0], allBounds[1], allBounds[2], allBounds[3],
                     allBounds[4], allBounds[5]);
    }

  icetEnable(ICET_DISPLAY);
  icetEnable(ICET_DISPLAY_INFLATE);
  if (this->DataReplicatedOnAllProcesses)
    {
    icetDataReplicationGroupColor(1);
    }
  else
    {
    icetDataReplicationGroupColor(
      static_cast<GLint>(this->Controller->GetLocalProcessId()));
    }

  // IceT will use the full render window.  We'll move images back where they
  // belong later.
  //int *size = render_state->GetRenderer()->GetVTKWindow()->GetActualSize();
  //glViewport(0, 0, size[0], size[1]);
  //glDisable(GL_SCISSOR_TEST);
  glClearColor((GLclampf)(0.0), (GLclampf)(0.0),
    (GLclampf)(0.0), (GLclampf)(0.0));
  glClearDepth(static_cast<GLclampf>(1.0));
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  icetEnable(ICET_CORRECT_COLORED_BACKGROUND);
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::CleanupContext(const vtkRenderState*)
{
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::Render(const vtkRenderState* render_state)
{
  this->IceTContext->SetController(this->Controller);
  if (!this->IceTContext->IsValid())
    {
    vtkErrorMacro("Could not initialize IceT context.");
    return;
    }

  this->SetupContext(render_state);

  icetDrawFunc(IceTDrawCallback);
  IceTDrawCallbackHandle = this;
  IceTDrawCallbackState = render_state;
  icetDrawFrame();
  IceTDrawCallbackHandle = NULL;
  IceTDrawCallbackState = NULL;

  this->CleanupContext(render_state);
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::Draw(const vtkRenderState* render_state)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (this->RenderPass)
    {
    this->RenderPass->Render(render_state);
    }
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::UpdateTileInformation(
  const vtkRenderState* render_state)
{
  int tile_size[2];
  double viewport[4] = {0, 0, 1, 1};
  if (render_state->GetFrameBuffer())
    {
    render_state->GetFrameBuffer()->GetLastSize(tile_size);
    }
  else
    {
    vtkWindow* window = render_state->GetRenderer()->GetVTKWindow();
    tile_size[0] = window->GetSize()[0];
    tile_size[1] = window->GetSize()[1];
    render_state->GetRenderer()->GetViewport(viewport);
    }

  vtkSmartPointer<vtkTilesHelper> tilesHelper = vtkSmartPointer<vtkTilesHelper>::New();
  tilesHelper->SetTileDimensions(this->TileDimensions);
  tilesHelper->SetTileMullions(this->TileMullions);
  tilesHelper->SetTileWindowSize(tile_size);

  int rank = this->Controller->GetLocalProcessId();
  const int *my_tile_viewport = tilesHelper->GetTileViewport(viewport,
    rank);

  if (!my_tile_viewport)
    {
    cout << "********* nothing to render." << endl;
    return;
    }

  if (this->LastTileMullions[0] == this->TileMullions[0] &&
    this->LastTileMullions[1] == this->TileMullions[1] &&
    this->LastTileDimensions[0] == this->TileDimensions[0] &&
    this->LastTileDimensions[1] == this->TileDimensions[1] &&
    this->LastTileViewport[0] == my_tile_viewport[0] &&
    this->LastTileViewport[1] == my_tile_viewport[1] &&
    this->LastTileViewport[2] == my_tile_viewport[2] &&
    this->LastTileViewport[3] == my_tile_viewport[3])
    {
    // No need to update the tile parameters.
    return;
    }

  double image_reduction_factor = this->ImageReductionFactor;

  cout << "_------------------" << endl;
  icetResetTiles();
  for (int x=0; x < this->TileDimensions[0]; x++)
    {
    for (int y=0; y < this->TileDimensions[1]; y++)
      {
      int cur_rank  = y * this->TileDimensions[0] + x;
      const int* tile_viewport = tilesHelper->GetTileViewport(viewport, cur_rank);
      if (!tile_viewport)
        {
        continue;
        }
      cout << cur_rank << " : "
        << tile_viewport[0]/image_reduction_factor << ", "
        << tile_viewport[1]/image_reduction_factor << ", "
        << tile_viewport[2]/image_reduction_factor << ", "
        << tile_viewport[3]/image_reduction_factor << endl;

      icetAddTile(
        tile_viewport[0]/image_reduction_factor, tile_viewport[1]/image_reduction_factor,
        (tile_viewport[2] - tile_viewport[0])/image_reduction_factor,
        (tile_viewport[3] - tile_viewport[1])/image_reduction_factor,
        cur_rank);
      // setting this should be needed so that the 2d actors work correctly.
      // However that messes up the tile-displays esp. when
      // image_reduction_factor > 1.
      //if (cur_rank == rank)
      //  {
      //  render_state->GetRenderer()->GetVTKWindow()->SetTileViewport(
      //    tile_viewport[0]/(double) tile_size[0],
      //    tile_viewport[1]/(double) tile_size[1],
      //    tile_viewport[2]/(double) tile_size[0],
      //    tile_viewport[3]/(double) tile_size[1]);
      //  }
      }
    }

  this->LastTileMullions[0] = this->TileMullions[0];
  this->LastTileMullions[1] = this->TileMullions[1];
  this->LastTileDimensions[0] = this->TileDimensions[0];
  this->LastTileDimensions[1] = this->TileDimensions[1];
  this->LastTileViewport[0] = my_tile_viewport[0];
  this->LastTileViewport[1] = my_tile_viewport[1];
  this->LastTileViewport[2] = my_tile_viewport[2];
  this->LastTileViewport[3] = my_tile_viewport[3];
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::GetLastRenderedTile(
  vtkSynchronizedRenderers::vtkRawImage& tile)
{
  tile.MarkInValid();

  GLint color_format;
  icetGetIntegerv(ICET_COLOR_FORMAT, &color_format);
  int *physicalViewport = this->LastTileViewport;
  int width  = physicalViewport[2] - physicalViewport[0];
  int height = physicalViewport[3] - physicalViewport[1];

  if (width < 1 || height < 1)
    {
    return;
    }

  tile.Resize(width, height, 4);

  // Copy as 4-bytes.  It's faster.
  GLuint *dest = (GLuint *)tile.GetRawPtr()->GetVoidPointer(0);
  GLuint *src = (GLuint *)icetGetColorBuffer();

  if (color_format == GL_RGBA)
    {
    memcpy(dest, src, sizeof(GLuint)*width*height);
    tile.MarkValid();
    }
  else if (static_cast<GLenum>(color_format) == vtkgl::BGRA)
    {
    for (int j = 0; j < height; j++)
      {
      for (int i = 0; i < width; i++)
        {
        dest[0] = src[2];
        dest[1] = src[1];
        dest[2] = src[0];
        dest[3] = src[3];
        dest += 4;  src += 4;
        }
      }
    tile.MarkValid();
    }
  else
    {
    vtkErrorMacro("ICE-T using unknown image format.");
    }
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
