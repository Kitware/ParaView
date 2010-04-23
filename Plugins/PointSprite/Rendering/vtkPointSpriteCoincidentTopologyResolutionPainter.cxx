/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSpriteCoincidentTopologyResolutionPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPointSpriteCoincidentTopologyResolutionPainter.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkMapper.h" // for VTK_RESOLVE_*
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPointSpriteCoincidentTopologyResolutionPainter);

//-----------------------------------------------------------------------------
vtkPointSpriteCoincidentTopologyResolutionPainter::
vtkPointSpriteCoincidentTopologyResolutionPainter()
{
  
}

//-----------------------------------------------------------------------------
vtkPointSpriteCoincidentTopologyResolutionPainter::
~vtkPointSpriteCoincidentTopologyResolutionPainter()
{
}

//-----------------------------------------------------------------------------
void vtkPointSpriteCoincidentTopologyResolutionPainter::RenderInternal(
   vtkRenderer *renderer,
   vtkActor *actor,
   unsigned long typeflags,
    bool forceCompileOnly)
{
  this->ResolveCoincidentTopology = VTK_RESOLVE_OFF;

  this->Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);
}

//-----------------------------------------------------------------------------
void vtkPointSpriteCoincidentTopologyResolutionPainter::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
