/*=========================================================================

  Program:   ParaView
  Module:    vtkPVColorMaterialHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVColorMaterialHelper - a helper to assist in similating the
// ColorMaterial behaviour of the default OpenGL pipeline.
// .SECTION Description
// vtkPVColorMaterialHelper is a helper to assist in similating the
// ColorMaterial behaviour of the default OpenGL pipeline. Look at
// vtkPVColorMaterialHelper_s for available GLSL functions.

#ifndef vtkPVColorMaterialHelper_h
#define vtkPVColorMaterialHelper_h

#include "vtkObject.h"

class vtkShaderProgram2;

class VTK_EXPORT vtkPVColorMaterialHelper : public vtkObject
{
public:
  static vtkPVColorMaterialHelper* New();
  vtkTypeMacro(vtkPVColorMaterialHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Initialize(vtkShaderProgram2*);
  vtkGetObjectMacro(Shader, vtkShaderProgram2);

  // Description:
  // Prepares the shader i.e. reads color material paramters state from OpenGL.
  // This must be called before the shader is bound.
  void PrepareForRendering();

  // Description:
  // Uploads any uniforms needed. This must be called only
  // after the shader has been bound, but before rendering the geometry.
  void Render();

protected:
  vtkPVColorMaterialHelper();
  ~vtkPVColorMaterialHelper();

  void SetShader(vtkShaderProgram2*);
  vtkShaderProgram2* Shader;

  enum eMaterialParamater
  {
    DISABLED = 0,
    AMBIENT = 1,
    DIFFUSE = 2,
    SPECULAR = 3,
    AMBIENT_AND_DIFFUSE = 4,
    EMISSION = 5
  };
  eMaterialParamater Mode;

private:
  vtkPVColorMaterialHelper(const vtkPVColorMaterialHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVColorMaterialHelper&) VTK_DELETE_FUNCTION;
};

#endif
