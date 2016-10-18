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

#ifndef vtkPointSpriteCoincidentTopologyResolutionPainter_h
#define vtkPointSpriteCoincidentTopologyResolutionPainter_h

#include "vtkCoincidentTopologyResolutionPainter.h"
#include "vtkPointSpriteRenderingModule.h" //needed for exports

class VTKPOINTSPRITERENDERING_EXPORT vtkPointSpriteCoincidentTopologyResolutionPainter
  : public vtkCoincidentTopologyResolutionPainter
{
public:
  static vtkPointSpriteCoincidentTopologyResolutionPainter* New();
  vtkTypeMacro(
    vtkPointSpriteCoincidentTopologyResolutionPainter, vtkCoincidentTopologyResolutionPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPointSpriteCoincidentTopologyResolutionPainter();
  ~vtkPointSpriteCoincidentTopologyResolutionPainter();

  // Description:
  // Performs the actual rendering. Override the behavior to force
  // the PolygonOffset mode. ZShift is buggy with the point sprite raycasting.
  virtual void RenderInternal(
    vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags, bool forceCompileOnly);

private:
  vtkPointSpriteCoincidentTopologyResolutionPainter(
    const vtkPointSpriteCoincidentTopologyResolutionPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPointSpriteCoincidentTopologyResolutionPainter&) VTK_DELETE_FUNCTION;
};

#endif
