/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIceTRenderer.h
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

protected:
  vtkIceTRenderer();
  virtual ~vtkIceTRenderer();

  virtual int UpdateCamera();
  virtual int UpdateGeometry();

  int ComposeNextFrame;

private:
  vtkIceTRenderer(const vtkIceTRenderer&); // Not implemented
  void operator=(const vtkIceTRenderer&); // Not implemented
};

#endif //__vtkIceTRenderer_h
