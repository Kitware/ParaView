//=========================================================================
//
//  Program:   ParaView
//  Module:    vtkPVColorMaterialHelper_vs.glsl
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

// symbols beginning with GL_ are reserved, so use lower-case
// instead
#define gl_ambient 1
#define gl_diffuse 2
#define gl_specular 3
#define gl_ambient_and_diffuse 4
#define gl_emission 5

uniform int vtkPVColorMaterialHelper_Mode;
gl_MaterialParameters getMaterialParameters()
{
  if (vtkPVColorMaterialHelper_Mode == 0)
  {
    return gl_FrontMaterial;
  }

  gl_MaterialParameters materialParams = gl_FrontMaterial;
  if (vtkPVColorMaterialHelper_Mode == gl_ambient)
  {
    materialParams.ambient = gl_Color;
  }
  else if (vtkPVColorMaterialHelper_Mode == gl_diffuse)
  {
    materialParams.diffuse = gl_Color;
  }
  else if (vtkPVColorMaterialHelper_Mode == gl_specular)
  {
    materialParams.specular = gl_Color;
  }
  else if (vtkPVColorMaterialHelper_Mode == gl_ambient_and_diffuse)
  {
    materialParams.ambient = gl_Color;
    materialParams.diffuse = gl_Color;
  }
  else if (vtkPVColorMaterialHelper_Mode == gl_emission)
  {
    materialParams.emission = gl_Color;
  }
  return materialParams;
}
