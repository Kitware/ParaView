/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Texture_vs.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Texture_vs.glsl
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

uniform float MaxPixelSize;
uniform vec2  viewport;

float GetRadius();

void propFuncVS()
{
  float radius = GetRadius();

  gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;
  gl_Position   = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
  //
  // Convert position to window coordinates
  //

  //
  // Convert Radius to window coordinates
  // radius/w is homogenous clip coord
  //
  float pixelSize  = (radius/gl_Position.w)*(4.0*viewport.y);

  // Clamp radius to prevent overloading if bad scalars were passed in
  if (pixelSize>MaxPixelSize)
    pixelSize = MaxPixelSize;

  gl_PointSize  = pixelSize;
  gl_FrontColor = gl_Color;
}
