/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIceTRenderer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkIceTRenderer.h"

#include <vtkObjectFactory.h>
#include <vtkLightCollection.h>

#include <GL/ice-t.h>

#define VTK41 1

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

vtkCxxRevisionMacro(vtkIceTRenderer, "1.2.2.2");
vtkStandardNewMacro(vtkIceTRenderer);

vtkIceTRenderer::vtkIceTRenderer()
{
  this->ComposeNextFrame = 0;
}

vtkIceTRenderer::~vtkIceTRenderer()
{
}

void vtkIceTRenderer::ComputeAspect()
{
  this->Superclass::ComputeAspect();

  if (! this->ComposeNextFrame)
    {
    return;
    }

  float aspect[2];
  this->GetAspect(aspect);

  int global_viewport[4];
  int tile_width, tile_height;

  icetGetIntegerv(ICET_GLOBAL_VIEWPORT, global_viewport);
  icetGetIntegerv(ICET_TILE_MAX_WIDTH, &tile_width);
  icetGetIntegerv(ICET_TILE_MAX_HEIGHT, &tile_height);

  float global_aspect = (float)global_viewport[2]/(float)global_viewport[3];
  float tile_aspect = (float)tile_width/(float)tile_height;

  aspect[0] *= global_aspect/tile_aspect;

  this->Superclass::SetAspect(aspect);
}

void vtkIceTRenderer::DeviceRender()
{
  vtkDebugMacro("In vtkIceTRenderer::DeviceRender");

  //Update the Camera once.  ICE-T will shift the viewpoint around.
  this->vtkOpenGLRenderer::ClearLights();
  this->vtkOpenGLRenderer::UpdateCamera();

  //In this case, behave as if just a normal renderer.
  if (!this->ComposeNextFrame)
    {
    this->vtkOpenGLRenderer::DeviceRender();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    return;
    }

  //Just in case ICE-T decides we don't have to render, make sure we have
  //a light.  Some other things like interactors will expect it to be there.
  if (this->Lights->GetNumberOfItems() < 1)
    {
    vtkDebugMacro("No lights are on, creating one.");
    this->CreateLight();
    }

  //ICE-T works much better if it knows the bounds of the geometry.
  float allBounds[6];
  this->ComputeVisiblePropBounds(allBounds);
  //Try to detect when bounds are empty and try to let ICE-T know that
  //nothing is in bounds.
  if (allBounds[0] > allBounds[1])
    {
    float tmp = 1e38;
    icetBoundingVertices(1, ICET_FLOAT, 0, 1, &tmp);
    }
  else
    {
    icetBoundingBoxf(allBounds[0], allBounds[1], allBounds[2], allBounds[3],
         allBounds[4], allBounds[5]);
    }

  //Setup ICE-T callback function.  Note that this is not thread safe.
  currentRenderer = this;
  icetDrawFunc(draw);

  //Now tell ICE-T to render the frame.
  icetDrawFrame();

  //Pop the modelview matrix (because the camera pushed it).
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  //Don't compose the next frame unless we are told to.
  this->ComposeNextFrame = 0;
}

void vtkIceTRenderer::RenderWithoutCamera()
{
  vtkDebugMacro("In vtkIceTRenderer::RenderWithoutCamera()");

  //Won't actually set camera view because we overrode UpdateCamera
  this->vtkOpenGLRenderer::DeviceRender();
}

//Fake a camera update without actually changing any matrix.
//ICE-T set the correct projection.
int vtkIceTRenderer::UpdateCamera()
{
  vtkDebugMacro("In vtkIceTRenderer::UpdateCamera()");

  //Push the modelview matrix, because the vktOpenGLRenderer::DeviceRender()
  //method expects this to happen and will pop it later.
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  //It is also important to clear the screen in between each frame.
  this->Clear();

  return 1;
}

#define MI(r,c)  (c*4+r)
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
  int i;

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
    float *bounds = this->PropArray[i]->GetBounds();

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

  //Now render the props that are really visible.
#if VTK41
  this->NumberOfPropsRendered = 0;
#else
  this->NumberOfPropsRenderedAsGeometry = 0;
#endif
  for (i = 0; i < this->PropArrayCount; i++)
    {
    if (visible[i])
      {
#if VTK41
      this->NumberOfPropsRendered +=
  this->PropArray[i]->RenderOpaqueGeometry(this);
#else
      this->NumberOfPropsRenderedAsGeometry +=
  this->PropArray[i]->RenderOpaqueGeometry(this);
#endif
      }
    }
  for (i = 0; i < this->PropArrayCount; i++)
    {
    if (visible[i])
      {
#if VTK41
      this->NumberOfPropsRendered +=
  this->PropArray[i]->RenderTranslucentGeometry(this);
#else
      this->NumberOfPropsRenderedAsGeometry +=
  this->PropArray[i]->RenderTranslucentGeometry(this);
#endif
      }
    }

#if VTK41
  vtkDebugMacro("Rendered " << this->NumberOfPropsRendered
    << " actors");
#else
  vtkDebugMacro("Rendered " << this->NumberOfPropsRenderedAsGeometry
    << " actors");
#endif

  delete[] visible;
#if VTK41
  return this->NumberOfPropsRendered;
#else
  return this->NumberOfPropsRenderedAsGeometry;
#endif
}

void vtkIceTRenderer::PrintSelf(ostream &os, vtkIndent indent)
{
  this->vtkOpenGLRenderer::PrintSelf(os, indent);
  os << indent << "ComposeNextFrame: " << this->ComposeNextFrame << endl;
}

//******************************************************************
//Local function/method implementation.
//******************************************************************

extern "C" {
  static void draw(void) {
    currentRenderer->RenderWithoutCamera();
  }
}
