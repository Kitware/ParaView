/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTRenderer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIceTRenderer
// .SECTION Description
// The Renderer that needs to be used in conjunction with
// vtkIceTRenderManager object.
// .SECTION see also
// vtkIceTRenderManager

#ifndef __vtkIceTRenderer_h
#define __vtkIceTRenderer_h

#include "vtkOpenGLRenderer.h"

class VTK_EXPORT vtkIceTRenderer : public vtkOpenGLRenderer
{
public:
  static vtkIceTRenderer *New();
  vtkTypeRevisionMacro(vtkIceTRenderer, vtkOpenGLRenderer);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Override the regular device render.
  virtual void DeviceRender();

  // Description:
  // Basically a callback to be used internally.
  void RenderWithoutCamera();

  // Description:
  // Renders next frame with composition.  If not continually called, this
  // object behaves just like vtkOpenGLRenderer.  This is inteneded to be
  // called by vtkIceTRenderManager.
  vtkSetMacro(ComposeNextFrame, int);

  // Description:
  // Adds a corrective term for the tile aspect.
  virtual void ComputeAspect();

  // Description:
  // Ensures that the background has an ambient color of 0 when color blend
  // compositing is on.
  virtual void Clear();

  // Description:
  // Reset ComposeNextFrame between rendering each eye for stereo viewing
  void StereoMidpoint();

protected:
  vtkIceTRenderer();
  virtual ~vtkIceTRenderer();

  virtual int UpdateCamera();
  virtual int UpdateGeometry();

  int ComposeNextFrame;
  int InIceTRender;

  vtkIceTRenderer(const vtkIceTRenderer&); // Not implemented
  void operator=(const vtkIceTRenderer&); // Not implemented
};

#endif //__vtkIceTRenderer_h
