/*=========================================================================

  Program:   ParaView
  Module:    vtkPVColorMaterialHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVColorMaterialHelper.h"

#include "vtkObjectFactory.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkShaderProgram2.h"
#include "vtkUniformVariables.h"
#include "vtkgl.h"
extern const char* vtkPVColorMaterialHelper_vs;

vtkStandardNewMacro(vtkPVColorMaterialHelper);
vtkCxxSetObjectMacro(vtkPVColorMaterialHelper, Shader, vtkShaderProgram2);
//----------------------------------------------------------------------------
vtkPVColorMaterialHelper::vtkPVColorMaterialHelper()
{
  this->Shader = 0;
}

//----------------------------------------------------------------------------
vtkPVColorMaterialHelper::~vtkPVColorMaterialHelper()
{
  this->SetShader(0);
}

//----------------------------------------------------------------------------
void vtkPVColorMaterialHelper::Initialize(vtkShaderProgram2* pgm)
{
  if (this->Shader != pgm)
  {
    this->SetShader(pgm);
    if (pgm)
    {
      vtkShader2* s = vtkShader2::New();
      s->SetSourceCode(vtkPVColorMaterialHelper_vs);
      s->SetType(VTK_SHADER_TYPE_VERTEX);
      s->SetContext(pgm->GetContext());
      pgm->GetShaders()->AddItem(s);
      s->Delete();
    }
  }
}
//----------------------------------------------------------------------------
void vtkPVColorMaterialHelper::PrepareForRendering()
{
  if (!this->Shader)
  {
    vtkErrorMacro("Please Initialize() before calling PrepareForRendering().");
    return;
  }

  this->Mode = vtkPVColorMaterialHelper::DISABLED;
  if (glIsEnabled(GL_COLOR_MATERIAL))
  {
    GLint colorMaterialParameter;
    glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER, &colorMaterialParameter);
    switch (colorMaterialParameter)
    {
      case GL_AMBIENT:
        this->Mode = vtkPVColorMaterialHelper::AMBIENT;
        break;

      case GL_DIFFUSE:
        this->Mode = vtkPVColorMaterialHelper::DIFFUSE;
        break;

      case GL_SPECULAR:
        this->Mode = vtkPVColorMaterialHelper::SPECULAR;
        break;

      case GL_AMBIENT_AND_DIFFUSE:
        this->Mode = vtkPVColorMaterialHelper::AMBIENT_AND_DIFFUSE;
        break;

      case GL_EMISSION:
        this->Mode = vtkPVColorMaterialHelper::EMISSION;
        break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVColorMaterialHelper::Render()
{
  if (!this->Shader)
  {
    vtkErrorMacro("Please Initialize() before calling Render().");
    return;
  }

  int value = this->Mode;
  this->Shader->GetUniformVariables()->SetUniformi("vtkPVColorMaterialHelper_Mode", 1, &value);
}

//----------------------------------------------------------------------------
void vtkPVColorMaterialHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Shader: " << this->Shader << endl;
}
