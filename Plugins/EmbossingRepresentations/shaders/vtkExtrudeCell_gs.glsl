//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtrude_gs.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//VTK::PositionVC::Dec
//VTK::TCoord::Dec

out vec3 normalVCGSOutput;
in vec4 vertexMCVSOutput[];

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 MCVCMatrix;
uniform mat4 MCDCMatrix;
uniform mat3 normalMatrix;
uniform int PrimitiveIDOffset;

uniform vec2 scalarRange;
uniform float extrusionFactor;
uniform int normalizeData;
uniform bool basisVisibility;

uniform samplerBuffer textureExtrude;

void main()
{
  mat4 VCMCMatrix = inverse(MCVCMatrix); //should be an uniform
  mat4 DCMCMatrix = inverse(MCDCMatrix); //should be an uniform

  vec3 d1MC = (vertexMCVSOutput[1] - vertexMCVSOutput[0]).xyz;
  vec3 d2MC = (vertexMCVSOutput[2] - vertexMCVSOutput[0]).xyz;
  vec3 normalMC = normalize(cross(d1MC, d2MC));

  // compute factor
  float factor = texelFetch(textureExtrude, gl_PrimitiveIDIn + PrimitiveIDOffset).r;

  bool nullFactor = false;

  // In case of normalizeData, we bound the value to [0, extrusionFactor]
  // 1% is added to avoid z-fighting
  if (normalizeData != 0)
    factor = 0.01 + clamp((factor - scalarRange.x) / (scalarRange.y - scalarRange.x), 0.0, 1.0);

  factor = factor * extrusionFactor;

  vec4 baseDC[3];
  baseDC[0] = MCDCMatrix * vertexMCVSOutput[0];
  baseDC[1] = MCDCMatrix * vertexMCVSOutput[1];
  baseDC[2] = MCDCMatrix * vertexMCVSOutput[2];

  vec4 dirMC = VCMCMatrix*normalize(MCVCMatrix*vec4(normalMC, 0.0));

  vec4 topDC[3];
  topDC[0] = baseDC[0] + MCDCMatrix*(dirMC) * factor;
  topDC[1] = baseDC[1] + MCDCMatrix*(dirMC) * factor;
  topDC[2] = baseDC[2] + MCDCMatrix*(dirMC) * factor;

  // if factor is negative, we need to correct the normals
  float factorSign = sign(factor);

  // in order to avoid z-fighting, we use a heuristic to consider that a factor is zero
  // if the extrusion is less than 1% of inscribed circle radius, no extrusion is applied
  float h = distance(topDC[0],baseDC[0]);
  float a = distance(baseDC[1],baseDC[0]);
  float b = distance(baseDC[2],baseDC[1]);
  float c = distance(baseDC[0],baseDC[2]);
  float s = 0.5*(a+b+c);
  float r = (s-a)*(s-b)*(s-c)/s;
  if (h < 0.01*r)
    factorSign = 0.0;

  vec3 surfDir = normalMatrix * normalMC;

  if (factorSign != 0.0)
    surfDir = -factorSign*surfDir;
  else
    surfDir = sign(extrusionFactor)*surfDir;

  // create base of prism
  if (basisVisibility)
  {
    for (int i = 0; i < 3; i++)
    {
      normalVCGSOutput = surfDir;
      //VTK::PrimID::Impl
      //VTK::TCoord::Impl
      //VTK::PositionVC::Impl
      gl_Position = baseDC[i];
      EmitVertex();
    }
    EndPrimitive();
  }

  if (factorSign == 0.0) return; // early exit

  // create top of prism
  for (int i = 0; i < 3; i++)
  {
    normalVCGSOutput = -surfDir;
    //VTK::PrimID::Impl
    //VTK::TCoord::Impl
    //VTK::PositionVC::Impl
    gl_Position = topDC[i];
    EmitVertex();
  }
  EndPrimitive();

  // create sides (quads) of prism
  for (int j = 0; j < 3; j++)
  {
    int i = j;
    int k = (j+1)%3;

    vec3 sideNormal = normalMatrix * normalize(cross((DCMCMatrix*(baseDC[k] - baseDC[i])).xyz, normalMC));

    normalVCGSOutput = sideNormal;
    //VTK::PrimID::Impl
    //VTK::TCoord::Impl
    //VTK::PositionVC::Impl
    gl_Position = topDC[i];
    EmitVertex();

    normalVCGSOutput = sideNormal;
    //VTK::PrimID::Impl
    //VTK::TCoord::Impl
    //VTK::PositionVC::Impl
    gl_Position = baseDC[i];
    EmitVertex();

    i = k;

    normalVCGSOutput = sideNormal;
    //VTK::PrimID::Impl
    //VTK::TCoord::Impl
    //VTK::PositionVC::Impl
    gl_Position = topDC[i];
    EmitVertex();

    normalVCGSOutput = sideNormal;
    //VTK::PrimID::Impl
    //VTK::TCoord::Impl
    //VTK::PositionVC::Impl
    gl_Position = baseDC[i];
    EmitVertex();

    EndPrimitive();
  }
}
