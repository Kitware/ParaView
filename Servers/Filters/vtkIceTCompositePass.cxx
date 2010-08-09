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
#include "vtkOpenGLRenderWindow.h"
#include "vtkPixelBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkTextureUnitManager.h"
#include "vtkShaderProgram2.h"
#include "vtkUniformVariables.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"

#include <assert.h>
#include "vtkgl.h"
#include <GL/ice-t.h>

extern const char *vtkIceTCompositeZPassShader_fs;

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
  this->DepthOnly=false;

  this->PBO=0;
  this->ZTexture=0;
  this->Program=0;
}

//----------------------------------------------------------------------------
vtkIceTCompositePass::~vtkIceTCompositePass()
{
  if(this->PBO!=0)
    {
    vtkErrorMacro(<<"PixelBufferObject should have been deleted in ReleaseGraphicsResources().");
    }
  if(this->ZTexture!=0)
    {
    vtkErrorMacro(<<"ZTexture should have been deleted in ReleaseGraphicsResources().");
    }
  if(this->Program!=0)
    {
    this->Program->Delete();
    }

  this->SetKdTree(0);
  this->SetRenderPass(0);
  this->SetController(0);
  this->IceTContext->Delete();
  this->IceTContext = 0;
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::ReleaseGraphicsResources(vtkWindow* window)
{
  if(this->RenderPass!=0)
    {
    this->RenderPass->ReleaseGraphicsResources(window);
    }

  if(this->PBO!=0)
    {
    this->PBO->Delete();
    this->PBO=0;
    }
  if(this->ZTexture!=0)
    {
    this->ZTexture->Delete();
    this->ZTexture=0;
    }
  if(this->Program!=0)
    {
    this->Program->ReleaseGraphicsResources();
    }
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
    (this->KdTree && this->UseOrderedCompositing && !this->DepthOnly &&
     this->KdTree->GetNumberOfRegions() >=
     this->IceTContext->GetController()->GetNumberOfProcesses());

  GLenum flags;
  if(this->DepthOnly)
    {
    flags=ICET_DEPTH_BUFFER_BIT;
    }
  else
    {
    // If translucent geometry is present, then we should not include
    // ICET_DEPTH_BUFFER_BIT in  the input-buffer argument.
    if (use_ordered_compositing)
      {
      flags=ICET_COLOR_BUFFER_BIT;
      }
    else
      {
      flags=ICET_COLOR_BUFFER_BIT | ICET_DEPTH_BUFFER_BIT;
      }
    }
  icetInputOutputBuffers(flags,flags);

  icetEnable(ICET_FLOATING_VIEWPORT);
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

  if(this->DepthOnly)
    {
    GLuint *depthBuffer=icetGetDepthBuffer();
    // OpenGL code to copy it back
    // merly the code from vtkOpenGLRenderWindow::SetZbufferData() except that
    // the data are not float but unsigned int.

    // get the dimension of the buffer
    GLint id;
    icetGetIntegerv(ICET_TILE_DISPLAYED,&id);
    GLint ids;
    icetGetIntegerv(ICET_NUM_TILES,&ids);

    GLint *vp=new GLint[4*ids];

    icetGetIntegerv(ICET_TILE_VIEWPORTS,vp);

    GLint x=vp[4*id];
    GLint y=vp[4*id+1];
    GLint w=vp[4*id+2];
    GLint h=vp[4*id+3];
    delete[] vp;

    // pbo arguments.
    unsigned int dims[2];
    vtkIdType continuousInc[3];

    dims[0]=static_cast<unsigned int>(w);
    dims[1]=static_cast<unsigned int>(h);
    continuousInc[0]=0;
    continuousInc[1]=0;
    continuousInc[2]=0;

    vtkOpenGLRenderWindow *context=
      static_cast<vtkOpenGLRenderWindow *>(
        render_state->GetRenderer()->GetRenderWindow());

    if(this->PBO==0)
      {
      this->PBO=vtkPixelBufferObject::New();
      this->PBO->SetContext(context);
      }
    if(this->ZTexture==0)
      {
      this->ZTexture=vtkTextureObject::New();
      this->ZTexture->SetContext(context);
      }

    // client to PBO
    this->PBO->Upload2D(VTK_UNSIGNED_INT,depthBuffer,dims,1,continuousInc);

    // PBO to TO
    this->ZTexture->CreateDepth(dims[0],dims[1],vtkTextureObject::Native,
                                this->PBO);

    // TO to FB: apply TO on quad with special zcomposite fragment shader.
    glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_ALWAYS);

    if(this->Program==0)
      {
      this->CreateProgram(context);
      }

    vtkTextureUnitManager *tu=context->GetTextureUnitManager();
    int sourceId=tu->Allocate();
    this->Program->GetUniformVariables()->SetUniformi("depth",1,&sourceId);
    vtkgl::ActiveTexture(vtkgl::TEXTURE0+static_cast<GLenum>(sourceId));
    this->Program->Use();
    this->ZTexture->Bind();
    this->ZTexture->CopyToFrameBuffer(0,0,
                                      w-1,h-1,
                                      0,0,w,h);
    this->ZTexture->UnBind();
    this->Program->Restore();

    tu->Free(sourceId);
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);

    glPopAttrib();
    }
  this->CleanupContext(render_state);
}

// ----------------------------------------------------------------------------
void vtkIceTCompositePass::CreateProgram(vtkOpenGLRenderWindow *context)
{
  assert("pre: context_exists" && context!=0);
  assert("pre: Program_void" && this->Program==0);

  this->Program=vtkShaderProgram2::New();
  this->Program->SetContext(context);

  vtkShader2 *shader=vtkShader2::New();
  shader->SetContext(context);

  this->Program->GetShaders()->AddItem(shader);
  shader->Delete();
  shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
  shader->SetSourceCode(vtkIceTCompositeZPassShader_fs);
  this->Program->Build();
  if(this->Program->GetLastBuildStatus()!=VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("prog build failed");
    }

  assert("post: Program_exists" && this->Program!=0);
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::Draw(const vtkRenderState* render_state)
{
  if(!this->DepthOnly)
    {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
  else
    {
    glClear(GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    }
  if (this->RenderPass)
    {
    this->RenderPass->Render(render_state);
    }
  if(this->DepthOnly)
    {
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    }
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::UpdateTileInformation(
  const vtkRenderState* render_state)
{
  double image_reduction_factor = this->ImageReductionFactor > 0?
    this->ImageReductionFactor : 1.0;

  int tile_size[2];
  double viewport[4] = {0, 0, 1, 1};
  if (render_state->GetFrameBuffer())
    {
    render_state->GetFrameBuffer()->GetLastSize(tile_size);
    }
  else
    {
    vtkWindow* window = render_state->GetRenderer()->GetVTKWindow();
    // NOTE: GetActualSize() does not include the TileScale.
    tile_size[0] = window->GetActualSize()[0];
    tile_size[1] = window->GetActualSize()[1];
    render_state->GetRenderer()->GetViewport(viewport);
    }

  vtkSmartPointer<vtkTilesHelper> tilesHelper = vtkSmartPointer<vtkTilesHelper>::New();
  tilesHelper->SetTileDimensions(this->TileDimensions);
  tilesHelper->SetTileMullions(this->TileMullions);
  tilesHelper->SetTileWindowSize(tile_size);

  int rank = this->Controller->GetLocalProcessId();
  int my_tile_viewport[4];
  if (tilesHelper->GetTileViewport(viewport, rank, my_tile_viewport))
    {
    // LastTileViewport is the icet viewport for the current renderer (treating
    // all tiles as one large display).
    this->LastTileViewport[0] = my_tile_viewport[0]/image_reduction_factor;
    this->LastTileViewport[1] = my_tile_viewport[1]/image_reduction_factor;
    this->LastTileViewport[2] = my_tile_viewport[2]/image_reduction_factor;
    this->LastTileViewport[3] = my_tile_viewport[3]/image_reduction_factor;

    // PhysicalViewport is the viewport in the current render-window where the
    // renderer maps.
    tilesHelper->GetPhysicalViewport(viewport, rank, this->PhysicalViewport);
    }
  else
    {
    this->LastTileViewport[0] = this->LastTileViewport[1] =
      this->LastTileViewport[2] = this->LastTileViewport[3] = 0;
    this->PhysicalViewport[0] = this->PhysicalViewport[1] =
      this->PhysicalViewport[2] = this->PhysicalViewport[3] = 0.0;
    }
  cout << "Physical Viewport: "
    << this->PhysicalViewport[0] << ", "
    << this->PhysicalViewport[1] << ", "
    << this->PhysicalViewport[2] << ", "
    << this->PhysicalViewport[3] << endl;

  if (this->LastTileMullions[0] == this->TileMullions[0] &&
    this->LastTileMullions[1] == this->TileMullions[1] &&
    this->LastTileDimensions[0] == this->TileDimensions[0] &&
    this->LastTileDimensions[1] == this->TileDimensions[1])
    {
    // No need to update the tile parameters.
    //return;
    }


  cout << "_------------------" << endl;
  icetResetTiles();
  for (int x=0; x < this->TileDimensions[0]; x++)
    {
    for (int y=0; y < this->TileDimensions[1]; y++)
      {
      int cur_rank  = y * this->TileDimensions[0] + x;
      int tile_viewport[4];
      if (!tilesHelper->GetTileViewport(viewport, cur_rank, tile_viewport))
        {
        continue;
        }
      cout << this << "=" << cur_rank << " : "
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
      // However that messes up the tile-displays with tdy > 0
      // if (cur_rank == rank)
      //   {
      //   render_state->GetRenderer()->GetVTKWindow()->SetTileScale(this->TileDimensions);
      //   render_state->GetRenderer()->GetVTKWindow()->SetTileViewport(
      //     tile_viewport[0]/(double) (tile_size[0]*this->TileDimensions[0]),
      //     tile_viewport[1]/(double) (tile_size[1]*this->TileDimensions[1]),
      //     tile_viewport[2]/(double) (tile_size[0]*this->TileDimensions[0]),
      //     tile_viewport[3]/(double) (tile_size[1]*this->TileDimensions[1]));
      //   }
      }
    }

  this->LastTileMullions[0] = this->TileMullions[0];
  this->LastTileMullions[1] = this->TileMullions[1];
  this->LastTileDimensions[0] = this->TileDimensions[0];
  this->LastTileDimensions[1] = this->TileDimensions[1];
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::GetLastRenderedTile(
  vtkSynchronizedRenderers::vtkRawImage& tile)
{
  tile.MarkInValid();

  GLint color_format;
  icetGetIntegerv(ICET_COLOR_FORMAT, &color_format);
  int width  = this->LastTileViewport[2] - this->LastTileViewport[0];
  int height = this->LastTileViewport[3] - this->LastTileViewport[1];

  // FIXME: when image_reduction_factor > 1, we need to scale width and height
  // accordingly.

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
void vtkIceTCompositePass::IceTInflateAndDisplay(vtkRenderer* renderer)
{
  vtkSynchronizedRenderers::vtkRawImage image;
  this->GetLastRenderedTile(image);
  if (!image.IsValid())
    {
    cout << "------ no image to render" << endl;
    return;
    }

  // FIXME, use correct physical viewport for this renderer.
  double viewport[4];
  renderer->GetViewport(viewport);
  renderer->SetViewport(0, 0, 1, 1);
  int tile_scale[2];
  renderer->GetVTKWindow()->GetTileScale(tile_scale);
  renderer->GetVTKWindow()->SetTileScale(1, 1);
  renderer->Clear();
  image.PushToViewport(renderer);
  renderer->GetVTKWindow()->SetTileScale(tile_scale);
  renderer->SetViewport(viewport);
}

//----------------------------------------------------------------------------
void vtkIceTCompositePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
