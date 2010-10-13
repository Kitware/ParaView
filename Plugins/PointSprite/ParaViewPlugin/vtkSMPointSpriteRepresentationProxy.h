/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPointSpriteRepresentationProxy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkSMPointSpriteRepresentationProxy - representation that can be used to
// show particle data in a 3D view
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>
// .SECTION Description
// vtkSMPointSpriteRepresentationProxy is a concrete representation that can be used
// to render points as spheres using a GPU mapper.
// This code doesn nothing since all we have done is changed the types in the
// xml description to create sprite mappers etc.

#ifndef __vtkSMPointSpriteRepresentationProxy_h
#define __vtkSMPointSpriteRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class vtkSMProperty;

class vtkSMPointSpriteRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMPointSpriteRepresentationProxy* New();
  vtkTypeMacro(vtkSMPointSpriteRepresentationProxy, vtkSMRepresentationProxy);

  // Description:
  // Initialize the constant radius, radius range and transfer functions if not initialized yet.
  static void InitializeDefaultValues(vtkSMProxy*);

protected:
   vtkSMPointSpriteRepresentationProxy();
  ~vtkSMPointSpriteRepresentationProxy();

  static void  InitializeTableValues(vtkSMProperty*);
  static void  InitializeSpriteTextures(vtkSMProxy* repr);

private:
  vtkSMPointSpriteRepresentationProxy(const vtkSMPointSpriteRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPointSpriteRepresentationProxy&); // Not implemented
};

#endif

