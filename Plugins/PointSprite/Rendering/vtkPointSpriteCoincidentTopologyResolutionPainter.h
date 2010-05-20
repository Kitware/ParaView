/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSpriteCoincidentTopologyResolutionPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointSpriteCoincidentTopologyResolutionPainter
// .SECTION Description
// Implementation for vtkCoincidentTopologyResolutionPainter using OpenGL.

#ifndef __vtkOpenGLCoincidentTopologyResolutionPainter_h
#define __vtkOpenGLCoincidentTopologyResolutionPainter_h

#include "vtkCoincidentTopologyResolutionPainter.h"

class VTK_EXPORT vtkPointSpriteCoincidentTopologyResolutionPainter :
  public vtkCoincidentTopologyResolutionPainter
{
public:
  static vtkPointSpriteCoincidentTopologyResolutionPainter* New();
  vtkTypeMacro(vtkPointSpriteCoincidentTopologyResolutionPainter,
    vtkCoincidentTopologyResolutionPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPointSpriteCoincidentTopologyResolutionPainter();
  ~vtkPointSpriteCoincidentTopologyResolutionPainter();

  // Description:
  // Performs the actual rendering. Override the behavior to force
  // the PolygonOffset mode. ZShift is buggy with the point sprite raycasting.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
                              unsigned long typeflags, bool forceCompileOnly);

private:
  vtkPointSpriteCoincidentTopologyResolutionPainter(
    const vtkPointSpriteCoincidentTopologyResolutionPainter&); // Not implemented.
  void operator=(const vtkPointSpriteCoincidentTopologyResolutionPainter&); // Not implemented.
};


#endif
