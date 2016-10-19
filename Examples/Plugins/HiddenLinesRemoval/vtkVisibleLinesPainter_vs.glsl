//=========================================================================
//
//  Program:   ParaView
//  Module:    vtkVisibleLinesPainter_vs.glsl
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================
// Id: Id

#version 120
varying vec4 vColor;

// from vtkPVColorMaterialHelper
gl_MaterialParameters getMaterialParameters();

// from vtkPVLightingHelper
vec4 singleColor(gl_MaterialParameters m, vec3 surfacePosEyeCoords, vec3 n);

vec4 colorFrontFace()
{
  vec4 heyeCoords = gl_ModelViewMatrix * gl_Vertex;
  vec3 eyeCoords = heyeCoords.xyz / heyeCoords.w;
  vec3 n = normalize(gl_NormalMatrix * gl_Normal);
  return singleColor(getMaterialParameters(), eyeCoords, n);
}

void main(void)
{
  gl_Position = ftransform();
  vColor = colorFrontFace();
}
