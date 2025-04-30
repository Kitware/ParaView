// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLBumpMapMapperDelegator.h"
#include "vtkBumpMapMapper.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBatchedPolyDataMapper.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPolyData.h"
#include "vtkShaderProgram.h"

#include <chrono>

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
/**
 * The vtkOpenGLBatchedBumpMapMapper inherits indirectly from vtkOpenGLBatchedPolyDataMapper
 * and contains most of the rendering code specific to the vtkBumpMapMapper.
 */
class vtkOpenGLBatchedBumpMapMapper : public vtkOpenGLBatchedPolyDataMapper
{
public:
  static vtkOpenGLBatchedBumpMapMapper* New();
  vtkTypeMacro(vtkOpenGLBatchedBumpMapMapper, vtkOpenGLBatchedPolyDataMapper);

protected:
  vtkOpenGLBatchedBumpMapMapper() = default;
  ~vtkOpenGLBatchedBumpMapMapper() override = default;

  /**
   * Implementation of vertex shader and fragment shader
   */
  void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor*) override;

  /**
   * Build the VBO/IBO, called by UpdateBufferObjects
   */
  void AppendOneBufferObject(vtkRenderer* ren, vtkActor* act, GLBatchElement* glBatchElement,
    vtkIdType& vertex_offset, std::vector<unsigned char>& colors,
    std::vector<float>& norms) override;

  /**
   * Update uniforms of shaders
   */
  void SetShaderValues(
    vtkShaderProgram* prog, GLBatchElement* glBatchElement, size_t primOffset) override;

private:
  vtkOpenGLBatchedBumpMapMapper(const vtkOpenGLBatchedBumpMapMapper&) = delete;
  void operator=(const vtkOpenGLBatchedBumpMapMapper&) = delete;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLBatchedBumpMapMapper);

//-------------------------------------------------------------------------
void vtkOpenGLBatchedBumpMapMapper::AppendOneBufferObject(vtkRenderer* ren, vtkActor* act,
  GLBatchElement* glBatchElement, vtkIdType& vertex_offset, std::vector<unsigned char>& colors,
  std::vector<float>& norms)
{
  vtkInformation* info = this->GetInputArrayInformation(0);
  int asso = info->Get(vtkDataObject::FIELD_ASSOCIATION());
  if (asso == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    vtkDataArray* scalars = this->GetInputArrayToProcess(0, this->CurrentInput);

    if (scalars)
    {
      if (scalars->GetDataType() != VTK_ID_TYPE)
      {
        // create "scalar" attribute on vertex buffer
        this->VBOs->AppendDataArray("scalar", scalars, scalars->GetDataType());
      }
      else
      {
        vtkErrorMacro(<< "Data type selected for extrusion is currently ID type which is not "
                         "supported (64bit type)");
      }
    }
  }

  this->Superclass::AppendOneBufferObject(ren, act, glBatchElement, vertex_offset, colors, norms);
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedBumpMapMapper::ReplaceShaderValues(
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
void vtkOpenGLBatchedBumpMapMapper::SetShaderValues(
  vtkShaderProgram* prog, GLBatchElement* glBatchElement, size_t primOffset)
{
  this->Superclass::SetShaderValues(prog, glBatchElement, primOffset);

  vtkBumpMapMapper* parent = static_cast<vtkBumpMapMapper*>(this->Parent);
  prog->SetUniformf("BumpMappingFactor", parent->GetBumpMappingFactor());
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLBumpMapMapperDelegator);

//------------------------------------------------------------------------------
vtkOpenGLBumpMapMapperDelegator::vtkOpenGLBumpMapMapperDelegator()
{
  if (this->Delegate != nullptr)
  {
    // delete the delegate created by parent class
    this->Delegate = nullptr;
  }
  // create our own.
  this->GLDelegate = vtkOpenGLBatchedBumpMapMapper::New();
  this->Delegate = vtk::TakeSmartPointer(this->GLDelegate);
}

//------------------------------------------------------------------------------
vtkOpenGLBumpMapMapperDelegator::~vtkOpenGLBumpMapMapperDelegator() = default;

//------------------------------------------------------------------------------
void vtkOpenGLBumpMapMapperDelegator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkOpenGLBumpMapMapperDelegator::ShallowCopy(vtkCompositePolyDataMapper* cpdm)
{
  this->Superclass::ShallowCopy(cpdm);
  // copy input array to delegate
  this->GLDelegate->SetInputArrayToProcess(0, cpdm->GetInputArrayInformation(0));
}

VTK_ABI_NAMESPACE_END
