// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIceTRenderer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/
// .NAME vtkIceTRenderer
// .SECTION Description
// The Renderer that needs to be used in conjunction with
// vtkIceTRenderManager object.
// .SECTION see also
// vtkIceTRenderManager

#ifndef __vtkIceTRenderer_h
#define __vtkIceTRenderer_h

//#include "vtksnlParallelWin32Header.h"

#include <vtkOpenGLRenderer.h>

#include <GL/ice-t.h>

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

protected:
  vtkIceTRenderer();
  virtual ~vtkIceTRenderer();

  virtual int UpdateCamera();
  virtual int UpdateGeometry();

  int ComposeNextFrame;
  int InIceTRender;
};

#endif //__vtkIceTRenderer_h
