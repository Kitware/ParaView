/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLightingHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLightingHelper - helper to assist in simulating lighting similar
// to default OpenGL pipeline.
// .SECTION Description
// vtkPVLightingHelper is an helper to assist in simulating lighting similar
// to default OpenGL pipeline. Look at vtkPVLightingHelper_s for available
// GLSL functions.

#ifndef vtkPVLightingHelper_h
#define vtkPVLightingHelper_h

#include "vtkObject.h"
#include "vtkShader2.h" // for vtkShader2Type

class vtkShaderProgram2;

class VTK_EXPORT vtkPVLightingHelper : public vtkObject
{
public:
  static vtkPVLightingHelper* New();
  vtkTypeMacro(vtkPVLightingHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the shader program to which we want to add the lighting kernels.
  // mode = VTK_SHADER_TYPE_VERTEX or VTK_SHADER_TYPE_FRAGMENT
  // depending on whether the vertex lighting or fragment lighting is to be
  // used.
  void Initialize(vtkShaderProgram2* shader, vtkShader2Type mode);
  vtkGetObjectMacro(Shader, vtkShaderProgram2);

  // Description:
  // Updates any lighting specific information needed.
  // This must be called before the shader program is bound.
  void PrepareForRendering();

protected:
  vtkPVLightingHelper();
  ~vtkPVLightingHelper();

  void SetShader(vtkShaderProgram2* shader);
  vtkShaderProgram2* Shader;

private:
  vtkPVLightingHelper(const vtkPVLightingHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVLightingHelper&) VTK_DELETE_FUNCTION;
};

#endif
