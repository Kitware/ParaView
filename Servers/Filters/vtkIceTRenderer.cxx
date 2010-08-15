/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTRenderer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/* -*- c++ -*- *******************************************************/

/* Id */

#include "vtkIceTRenderer.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkHardwareSelector.h"
#include "vtkIceTContext.h"
#include "vtkIntArray.h"
#include "vtkLightCollection.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"

#include <GL/ice-t.h>

#include <vtkstd/algorithm>

//******************************************************************
// Prototypes
//******************************************************************

extern "C"{
  static void draw(void);
}
static vtkIceTRenderer *currentRenderer;


//******************************************************************
// vtkIceTRenderer implementation.
//******************************************************************

vtkStandardNewMacro(vtkIceTRenderer);

vtkCxxSetObjectMacro(vtkIceTRenderer, SortingKdTree, vtkPKdTree);
vtkCxxSetObjectMacro(vtkIceTRenderer, DataReplicationGroup, vtkIntArray);

vtkIceTRenderer::vtkIceTRenderer()
{
  this->ComposeNextFrame = 0;
  this->InIceTRender = 0;

  this->Strategy = vtkIceTRenderManager::DEFAULT;
  this->ComposeOperation = vtkIceTRenderManager::ComposeOperationClosest;

  this->SortingKdTree = NULL;

  this->DataReplicationGroup = NULL;

  this->Context = vtkIceTContext::New();
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->PropVisibility = 0;
  this->CollectDepthBuffer = 0;

  for ( int i = 0; i < 4; ++ i )
    {
    this->PhysicalViewport[i] = 0;
    }

  // Disable the gradient and texuted background color for now.
  this->Superclass::TexturedBackgroundOff();
  this->Superclass::GradientBackgroundOff();
}

vtkIceTRenderer::~vtkIceTRenderer()
{
  this->SetSortingKdTree(NULL);
  this->SetDataReplicationGroup(NULL);

  this->Context->Delete();
}

//-----------------------------------------------------------------------------

void vtkIceTRenderer::SetController(vtkMultiProcessController *controller)
{
  if (controller == this->Context->GetController())
    {
    return;
    }

  this->Context->SetController(controller);

  if (controller)
    {
    vtkIntArray *drg = vtkIntArray::New();
    drg->SetNumberOfComponents(1);
    drg->SetNumberOfTuples(1);
    drg->SetValue(0, controller->GetLocalProcessId());
    this->SetDataReplicationGroup(drg);
    drg->Delete();
    }
  else
    {
    this->SetDataReplicationGroup(NULL);
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderer::SetDataReplicationGroupColor(int color)
{
  // Just use ICE-T to figure out groups, since it can do that already.
  this->Context->MakeCurrent();

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

void vtkIceTRenderer::GetTiledSizeAndOrigin(int *width, int *height,
                                            int *lowerLeftX, int *lowerLeftY)
{
  if (this->InIceTRender)
    {
    // If this method is being called during an IceT render, then IceT has
    // modified the view to cover the full tile (rendering context).  Report as
    // such.
    int *size = this->VTKWindow->GetActualSize();
    *width = size[0];
    *height = size[1];

    *lowerLeftX = 0;  *lowerLeftY = 0;
    }
  else
    {
    // If this method is called outside of an IceT render, fool other classes
    // into thinking the entire tiled display is of one.  IceT will take care of
    // the details of splitting it up later.
    double viewport[4];
    this->GetViewport(viewport);
    this->NormalizedDisplayToDisplay(viewport[0], viewport[1]);
    this->NormalizedDisplayToDisplay(viewport[2], viewport[3]);

    *lowerLeftX = (int)(viewport[0]+0.5);
    *lowerLeftY = (int)(viewport[1]+0.5);
    *width = (int)(viewport[2]+0.5) - *lowerLeftX;
    *height = (int)(viewport[3]+0.5) - *lowerLeftY;
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderer::DeviceRender()
{
  vtkDebugMacro("In vtkIceTRenderer::DeviceRender");
  vtkTimerLog::MarkStartEvent("IceT Dev Render");

  //Update the Camera once.  ICE-T will shift the viewpoint around.
  this->Superclass::ClearLights();
  this->Superclass::UpdateCamera();

  //In this case, behave as if just a normal renderer.
  if (!this->ComposeNextFrame)
    {
    this->vtkOpenGLRenderer::DeviceRender();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    this->InvokeEvent(vtkCommand::EndEvent,NULL);
    vtkTimerLog::MarkEndEvent("IceT Dev Render");
    return;
    }

  // IceT will use the full render window.  We'll move images back where they
  // belong later.
  int *size = this->RenderWindow->GetActualSize();
  glViewport(0, 0, size[0], size[1]);
  glDisable(GL_SCISSOR_TEST);

  //Just in case ICE-T decides we don't have to render, make sure we have
  //a light.  Some other things like interactors will expect it to be there.
  if (this->Lights->GetNumberOfItems() < 1)
    {
    vtkDebugMacro("No lights are on, creating one.");
    this->CreateLight();
    }

  //Make this the current IceT context.
  this->Context->MakeCurrent();

  //Sync IceT state with this object's state.
  switch (this->Strategy)
    {
    case vtkIceTRenderManager::DEFAULT:icetStrategy(ICET_STRATEGY_REDUCE);break;
    case vtkIceTRenderManager::REDUCE: icetStrategy(ICET_STRATEGY_REDUCE);break;
    case vtkIceTRenderManager::VTREE:  icetStrategy(ICET_STRATEGY_VTREE); break;
    case vtkIceTRenderManager::SPLIT:  icetStrategy(ICET_STRATEGY_SPLIT); break;
    case vtkIceTRenderManager::SERIAL: icetStrategy(ICET_STRATEGY_SERIAL);break;
    case vtkIceTRenderManager::DIRECT: icetStrategy(ICET_STRATEGY_DIRECT);break;
    default: vtkErrorMacro("Invalid strategy set"); break;
    }
  switch (this->ComposeOperation)
    {
    case vtkIceTRenderManager::ComposeOperationClosest:
      icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT | ICET_DEPTH_BUFFER_BIT,
        this->CollectDepthBuffer?
        ICET_COLOR_BUFFER_BIT| ICET_DEPTH_BUFFER_BIT:
        ICET_COLOR_BUFFER_BIT);
      break;

    case vtkIceTRenderManager::ComposeOperationOver:
      icetInputOutputBuffers(ICET_COLOR_BUFFER_BIT, ICET_COLOR_BUFFER_BIT);
      break;
    default:
      vtkErrorMacro("Invalid compose operation set");
      break;
    }

  //Set up ordered compositing.
  if (   (this->ComposeOperation == vtkIceTRenderManager::ComposeOperationOver)
      && this->SortingKdTree
      && (   this->SortingKdTree->GetNumberOfRegions()
          >= this->Context->GetController()->GetNumberOfProcesses()) )
    {
    // Setup ICE-T context for correct sorting.
    icetEnable(ICET_ORDERED_COMPOSITE);
    vtkIntArray *orderedProcessIds = vtkIntArray::New();

    // Order all the regions.
    vtkCamera *camera = this->GetActiveCamera();
    if (camera->GetParallelProjection())
      {
      this->SortingKdTree->ViewOrderAllProcessesInDirection(
                                             camera->GetDirectionOfProjection(),
                                             orderedProcessIds);
      }
    else
      {
      this->SortingKdTree->ViewOrderAllProcessesFromPosition(
                                                          camera->GetPosition(),
                                                          orderedProcessIds);
      }

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

  // \NOTE: This suppose to fix the issue with volume rendering
  // on the tile display mode where every tile has to render and composite
  // but for some reason it didnot work.
//  icetDisable(ICET_FLOATING_VIEWPORT);

  //Make sure we tell ICE-T what the background color is.  If the background
  //is black, also make it transparent so that we can skip fixing it.
  GLint in_buffers;
  icetGetIntegerv(ICET_INPUT_BUFFERS, &in_buffers);
  if (   (in_buffers == ICET_COLOR_BUFFER_BIT)
      && (this->Background[0] == 0.0)
      && (this->Background[1] == 0.0)
      && (this->Background[2] == 0.0) )
    {
    glClearColor((GLclampf)(0.0), (GLclampf)(0.0),
                 (GLclampf)(0.0), (GLclampf)(0.0));
    }
  else
    {
    glClearColor((GLclampf)(this->Background[0]),
                 (GLclampf)(this->Background[1]),
                 (GLclampf)(this->Background[2]),
                 (GLclampf)(1.0));
    }

  //ICE-T works much better if it knows the bounds of the geometry.
  double allBounds[6];
  this->ComputeVisiblePropBounds(allBounds);
  //Try to detect when bounds are empty and try to let ICE-T know that
  //nothing is in bounds.
  if (allBounds[0] > allBounds[1])
    {
    float tmp = VTK_LARGE_FLOAT;
    icetBoundingVertices(1, ICET_FLOAT, 0, 1, &tmp);
    }
  else
    {
    icetBoundingBoxd(allBounds[0], allBounds[1], allBounds[2], allBounds[3],
                     allBounds[4], allBounds[5]);
    }

  //Setup ICE-T callback function.  Note that this is not thread safe.
  currentRenderer = this;
  icetDrawFunc(draw);

  //Now tell ICE-T to render the frame.
  this->InIceTRender = 1;
  icetDrawFrame();
  this->InIceTRender = 0;

  //Pop the modelview matrix (because the camera pushed it).
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  //Don't compose the next frame unless we are told to.
  this->ComposeNextFrame = 0;

  //I think this is necessary because UpdateGeometry() no longer calls
  //EndEvent.  That seems like a weird place to invoke EndEvent when
  //StartEvent is invoked in Render().
  this->InvokeEvent(vtkCommand::EndEvent, NULL);

  //This is also traditionally done in UpdateGeometry().
  this->RenderTime.Modified();

  vtkTimerLog::MarkEndEvent("IceT Dev Render");
}

//-----------------------------------------------------------------------------

void vtkIceTRenderer::Clear()
{
  if (!this->InIceTRender)
    {
    this->Superclass::Clear();
    return;
    }

  // Set clear color so that it is transparent if color blending.
  float bgcolor[4];
  icetGetFloatv(ICET_BACKGROUND_COLOR, bgcolor);
  vtkDebugMacro("Clear Color: " << bgcolor[0] << ", " << bgcolor[1]
                << ", " << bgcolor[2] << ", " << bgcolor[3]);
  glClearColor(bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3]);
  GLbitfield  clear_mask = GL_COLOR_BUFFER_BIT;
  if (!this->GetPreserveDepthBuffer())
    {
    glClearDepth(static_cast<GLclampf>(1.0));
    clear_mask |= GL_DEPTH_BUFFER_BIT;
    }
  glClear(clear_mask);
}

//-----------------------------------------------------------------------------

void vtkIceTRenderer::RenderWithoutCamera()
{
  vtkDebugMacro("In vtkIceTRenderer::RenderWithoutCamera()");

  //Won't actually set camera view because we overrode UpdateCamera
  this->Superclass::DeviceRender();
}

//-----------------------------------------------------------------------------

//Fake a camera update without actually changing any matrix.
//ICE-T set the correct projection.
int vtkIceTRenderer::UpdateCamera()
{
  vtkDebugMacro("In vtkIceTRenderer::UpdateCamera()");

  //Push the modelview matrix, because the vktOpenGLRenderer::DeviceRender()
  //method expects this to happen and will pop it later.
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  //It is also important to clear the screen in between each viewport render.
  this->Clear();

  return 1;
}

//-----------------------------------------------------------------------------

#define MI(r,c)    (c*4+r)
static inline void UpdateViewParams(GLdouble vert[3], GLdouble transform[16],
                                    bool &left, bool &right, bool &bottom,
                                    bool &top, bool &znear, bool &zfar)
{
  GLdouble x, y, z, w;

  w = transform[MI(3,0)]*vert[0] + transform[MI(3,1)]*vert[1]
    + transform[MI(3,2)]*vert[2] + transform[MI(3,3)];
  x = transform[MI(0,0)]*vert[0] + transform[MI(0,1)]*vert[1]
    + transform[MI(0,2)]*vert[2] + transform[MI(0,3)];
  if (x < w)  left   = true;
  if (x > -w) right  = true;
  y = transform[MI(1,0)]*vert[0] + transform[MI(1,1)]*vert[1]
    + transform[MI(1,2)]*vert[2] + transform[MI(1,3)];
  if (y < w)  bottom = true;
  if (y > -w) top    = true;
  z = transform[MI(2,0)]*vert[0] + transform[MI(2,1)]*vert[1]
    + transform[MI(2,2)]*vert[2] + transform[MI(2,3)];
  if (z < w)  znear  = true;
  if (z > -w) zfar   = true;
}

//Cull objects that are outside the current OpenGL view.
//Since ICE-T often renders projections that are a subsection of the
//overall viewing volume.
int vtkIceTRenderer::UpdateGeometry()
{
  vtkDebugMacro("In vtkIceTRenderer::UpdateGeometry()");

  if (this->Selector)
    {
    // When selector is present, we are performing a selection,
    // so do the selection rendering pass instead of the normal passes.
    // Delegate the rendering of the props to the selector itself.
    this->NumberOfPropsRendered = this->Selector->Render(this,
      this->PropArray, this->PropArrayCount);
    return this->NumberOfPropsRendered;
    }

  int i;

  this->NumberOfPropsRendered = 0;

  //First, get the current transformation.
  GLdouble projection[16];
  GLdouble modelview[16];
  GLdouble transform[16];
  glGetDoublev(GL_PROJECTION_MATRIX, projection);
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  for (int c = 0; c < 4; c++)
    {
    for (int r = 0; r < 4; r++)
      {
      transform[c*4+r] = (  projection[MI(r,0)]*modelview[MI(0,c)]
                          + projection[MI(r,1)]*modelview[MI(1,c)]
                          + projection[MI(r,2)]*modelview[MI(2,c)]
                          + projection[MI(r,3)]*modelview[MI(3,c)]);
      }
    }

  //Next, determine which props are really in view.
  bool *visible = new bool[this->PropArrayCount];
  for (i = 0; i < this->PropArrayCount; i++)
    {
    double *bounds = this->PropArray[i]->GetBounds();

    // I do not know why, but this prop can return a null bounds.
    // It may be the scalar bar.
    if (bounds)
      {
      bool left, right, bottom, top, znear, zfar;
      left = right = bottom = top = znear = zfar = false;

      GLdouble vert[3];

      vert[0] = bounds[0];  vert[1] = bounds[2];  vert[2] = bounds[4];
      UpdateViewParams(vert, transform, left, right, bottom, top, znear, zfar);
      vert[0] = bounds[0];  vert[1] = bounds[2];  vert[2] = bounds[5];
      UpdateViewParams(vert, transform, left, right, bottom, top, znear, zfar);
      vert[0] = bounds[0];  vert[1] = bounds[3];  vert[2] = bounds[4];
      UpdateViewParams(vert, transform, left, right, bottom, top, znear, zfar);
      vert[0] = bounds[0];  vert[1] = bounds[3];  vert[2] = bounds[5];
      UpdateViewParams(vert, transform, left, right, bottom, top, znear, zfar);
      vert[0] = bounds[1];  vert[1] = bounds[2];  vert[2] = bounds[4];
      UpdateViewParams(vert, transform, left, right, bottom, top, znear, zfar);
      vert[0] = bounds[1];  vert[1] = bounds[2];  vert[2] = bounds[5];
      UpdateViewParams(vert, transform, left, right, bottom, top, znear, zfar);
      vert[0] = bounds[1];  vert[1] = bounds[3];  vert[2] = bounds[4];
      UpdateViewParams(vert, transform, left, right, bottom, top, znear, zfar);
      vert[0] = bounds[1];  vert[1] = bounds[3];  vert[2] = bounds[5];
      UpdateViewParams(vert, transform, left, right, bottom, top, znear, zfar);

      visible[i] = left && right && bottom && top && znear && zfar;
      }
    else
      {
      // Can't check bounds.  Be conservative and assume it is visible.
      visible[i] = 1;
      }
    }

  //Now render the props that are really visible.
  for (i = 0; i < this->PropArrayCount; i++)
    {
    if (visible[i])
      {
      this->NumberOfPropsRendered +=
        this->PropArray[i]->RenderOpaqueGeometry(this);
      }
    }

  int hasTranslucentPolygonalGeometry=0;
  for ( i = 0; !hasTranslucentPolygonalGeometry && i < this->PropArrayCount;
    i++ )
    {
    if (visible[i])
      {
      hasTranslucentPolygonalGeometry=
        this->PropArray[i]->HasTranslucentPolygonalGeometry();
      }
    }
  if(hasTranslucentPolygonalGeometry)
    {
    this->PropVisibility = visible;
    this->DeviceRenderTranslucentPolygonalGeometry();
    this->PropVisibility = 0;
    }

  for (i = 0; i < this->PropArrayCount; i++)
    {
    if (visible[i])
      {
      this->NumberOfPropsRendered +=
        this->PropArray[i]->RenderVolumetricGeometry(this);
      }
    }

  for (i = 0; i < this->PropArrayCount; i++)
    {
    if (visible[i])
      {
      this->NumberOfPropsRendered +=
        this->PropArray[i]->RenderOverlay(this);
      }
    }

  vtkDebugMacro("Rendered " << this->NumberOfPropsRendered
                << " actors");

  delete[] visible;
  return this->NumberOfPropsRendered;
}

//-----------------------------------------------------------------------------
int vtkIceTRenderer::UpdateTranslucentPolygonalGeometry()
{
  int result=0;
  // loop through props and give them a chance to
  // render themselves as translucent geometry
  for (int i = 0; i < this->PropArrayCount; i++ )
    {
    if (this->PropVisibility && this->PropVisibility[i])
      {
      int rendered=this->PropArray[i]->RenderTranslucentPolygonalGeometry(this);
      this->NumberOfPropsRendered += rendered;
      result+=rendered;
      }
    }
  return result;
}

//-----------------------------------------------------------------------------

void vtkIceTRenderer::StereoMidpoint()
{
  this->ComposeNextFrame = 1;
}

//-----------------------------------------------------------------------------

double vtkIceTRenderer::GetRenderTime()
{
  if (this->Context->IsValid())
    {
    double t;
    this->Context->MakeCurrent();
    icetGetDoublev(ICET_RENDER_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}

//-----------------------------------------------------------------------------

double vtkIceTRenderer::GetImageProcessingTime()
{
  return this->GetBufferReadTime() + this->GetCompositeTime();
}

//-----------------------------------------------------------------------------

double vtkIceTRenderer::GetBufferReadTime()
{
  if (this->Context->IsValid())
    {
    double t;
    this->Context->MakeCurrent();
    icetGetDoublev(ICET_BUFFER_READ_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}

//-----------------------------------------------------------------------------

double vtkIceTRenderer::GetBufferWriteTime()
{
  if (this->Context->IsValid())
    {
    double t;
    this->Context->MakeCurrent();
    icetGetDoublev(ICET_BUFFER_WRITE_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}

//-----------------------------------------------------------------------------

double vtkIceTRenderer::GetCompositeTime()
{
  if (this->Context->IsValid())
    {
    double t;
    this->Context->MakeCurrent();
    icetGetDoublev(ICET_COMPOSITE_TIME, &t);
    return t;
    }
  else
    {
    return 0.0;
    }
}

//-----------------------------------------------------------------------------

void vtkIceTRenderer::PrintSelf(ostream &os, vtkIndent indent)
{
  this->vtkOpenGLRenderer::PrintSelf(os, indent);

  os << indent << "CollectDepthBuffer: " << this->CollectDepthBuffer << endl;
  os << indent << "ComposeNextFrame: " << this->ComposeNextFrame << endl;

  os << indent << "ICE-T Context: " << this->Context << endl;

  os << indent << "Strategy: ";
  switch (this->Strategy)
    {
    case vtkIceTRenderManager::DEFAULT: os << "DEFAULT"; break;
    case vtkIceTRenderManager::REDUCE:  os << "REDUCE";  break;
    case vtkIceTRenderManager::VTREE:   os << "VTREE";   break;
    case vtkIceTRenderManager::SPLIT:   os << "SPLIT";   break;
    case vtkIceTRenderManager::SERIAL:  os << "SERIAL";  break;
    case vtkIceTRenderManager::DIRECT:  os << "DIRECT";  break;
    }
  os << endl;

  os << indent << "Compose Operation: ";
  switch (this->ComposeOperation)
    {
    case vtkIceTRenderManager::ComposeOperationClosest:
      os << "closest to camera";
      break;
    case vtkIceTRenderManager::ComposeOperationOver:
      os << "Porter and Duff OVER operator";
      break;
    }
  os << endl;

  os << indent << "PhysicalViewport: "
     << this->PhysicalViewport[0] << " " << this->PhysicalViewport[1] << " "
     << this->PhysicalViewport[2] << " " << this->PhysicalViewport[3] << endl;

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

//******************************************************************
//Local function/method implementation.
//******************************************************************

extern "C" {
  static void draw(void) {
    currentRenderer->RenderWithoutCamera();
  }
}
