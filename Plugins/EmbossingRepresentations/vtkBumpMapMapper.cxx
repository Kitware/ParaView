/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBumpMapMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBumpMapMapper.h"

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHardwareSelector.h"
#include "vtkInformation.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPolyData.h"
#include "vtkShaderProgram.h"
#include "vtk_glew.h"

// this header have to be included after vtkHardwareSelector
#include "vtkCompositePolyDataMapper2Internal.h"

class vtkBumpMapMapperHelper : public vtkCompositeMapperHelper2
{
public:
  static vtkBumpMapMapperHelper* New();
  vtkTypeMacro(vtkBumpMapMapperHelper, vtkCompositeMapperHelper2);

protected:
  vtkBumpMapMapperHelper() = default;
  ~vtkBumpMapMapperHelper() override = default;

  /**
   * Implementation of vertex shader and fragment shader
   */
  void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor*) override;

  /**
   * Build the VBO/IBO, called by UpdateBufferObjects
   */
  void AppendOneBufferObject(vtkRenderer* ren, vtkActor* act, vtkCompositeMapperHelperData* hdata,
    vtkIdType& flat_index, std::vector<unsigned char>& colors, std::vector<float>& norms) override;

  /**
   * Update uniforms of shaders
   */
  void SetShaderValues(
    vtkShaderProgram* prog, vtkCompositeMapperHelperData* hdata, size_t primOffset) override;

private:
  vtkBumpMapMapperHelper(const vtkBumpMapMapperHelper&) = delete;
  void operator=(const vtkBumpMapMapperHelper&) = delete;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBumpMapMapperHelper);

//-------------------------------------------------------------------------
void vtkBumpMapMapperHelper::AppendOneBufferObject(vtkRenderer* ren, vtkActor* act,
  vtkCompositeMapperHelperData* hdata, vtkIdType& voffset, std::vector<unsigned char>& newColors,
  std::vector<float>& newNorms)
{
  vtkInformation* info = this->GetInputArrayInformation(0);
  int asso = info->Get(vtkDataObject::FIELD_ASSOCIATION());
  if (asso == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    vtkDataArray* scalars = this->GetInputArrayToProcess(0, this->CurrentInput);
    if (scalars)
    {
      // create "scalar" attribute on vertex buffer
      this->VBOs->AppendDataArray("scalar", scalars, scalars->GetDataType());
    }
  }

  this->Superclass::AppendOneBufferObject(ren, act, hdata, voffset, newColors, newNorms);
}

//-----------------------------------------------------------------------------
void vtkBumpMapMapperHelper::ReplaceShaderValues(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Dec",
    "//VTK::PositionVC::Dec\n" // other declarations will be done by the superclass
    "in float scalar;\n"
    "out float scalarVSOutput;\n");

  vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Impl",
    "//VTK::PositionVC::Impl\n" // other declarations will be done by the superclass
    "scalarVSOutput = scalar;\n");

  vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec",
    "//VTK::Normal::Dec\n" // other declarations will be done by the superclass
    "in float scalarVSOutput;\n"
    "uniform float BumpMappingFactor;\n"
    "#define HALF_PI 1.57079632679\n");

  vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl",
    "vec3 grad = vec3(dFdx(scalarVSOutput), dFdy(scalarVSOutput), 0.0);\n"
    "float magnitude = clamp(abs(BumpMappingFactor) * length(grad), 0.0, 1.0);\n"
    "// in case of null gradient, set a valid vector\n"
    "if (magnitude < 0.00001) grad=vec3(1.0,0.0,0.0);\n"
    "float angle = HALF_PI * (1.0 - magnitude);\n"
    "vec3 normalVCVSOutput = sign(BumpMappingFactor)*cos(angle)*normalize(grad) + vec3(0.0, 0.0, "
    "sin(angle));\n");

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);

  this->Superclass::ReplaceShaderValues(shaders, ren, actor);
}

//-----------------------------------------------------------------------------
void vtkBumpMapMapperHelper::SetShaderValues(
  vtkShaderProgram* prog, vtkCompositeMapperHelperData* hdata, size_t primOffset)
{
  this->Superclass::SetShaderValues(prog, hdata, primOffset);

  vtkBumpMapMapper* parent = static_cast<vtkBumpMapMapper*>(this->Parent);
  prog->SetUniformf("BumpMappingFactor", parent->GetBumpMappingFactor());
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBumpMapMapper);

//-----------------------------------------------------------------------------
void vtkBumpMapMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BumpMappingFactor: " << this->BumpMappingFactor << endl;
}

//-----------------------------------------------------------------------------
vtkCompositeMapperHelper2* vtkBumpMapMapper::CreateHelper()
{
  vtkCompositeMapperHelper2* helper = vtkBumpMapMapperHelper::New();

  // copy input array to helper
  helper->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));

  return helper;
}
