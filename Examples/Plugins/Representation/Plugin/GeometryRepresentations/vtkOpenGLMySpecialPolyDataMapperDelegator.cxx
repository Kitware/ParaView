// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLMySpecialPolyDataMapperDelegator.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBatchedPolyDataMapper.h"

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
/**
 * The vtkOpenGLBatchedMySpecialPolyDataMapper inherits indirectly from vtkOpenGLPolydataMapper
 * and can do GLSL string replacements and make OpenGL calls. For example, here, we override
 * ReplaceShaderColor, ReplaceShaderPositionVC and AppendOneBufferObject. These methods all
 * redirect back to parent class.
 */
class vtkOpenGLBatchedMySpecialPolyDataMapper : public vtkOpenGLBatchedPolyDataMapper
{
public:
  static vtkOpenGLBatchedMySpecialPolyDataMapper* New();
  vtkTypeMacro(vtkOpenGLBatchedMySpecialPolyDataMapper, vtkOpenGLBatchedPolyDataMapper);

protected:
  vtkOpenGLBatchedMySpecialPolyDataMapper() = default;
  ~vtkOpenGLBatchedMySpecialPolyDataMapper() override = default;

  void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderPositionVC(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor) override;
  void AppendOneBufferObject(vtkRenderer* ren, vtkActor* act, GLBatchElement* glBatchElement,
    vtkIdType& vertex_offset, std::vector<unsigned char>& colors,
    std::vector<float>& norms) override;

private:
  vtkOpenGLBatchedMySpecialPolyDataMapper(const vtkOpenGLBatchedMySpecialPolyDataMapper&) = delete;
  void operator=(const vtkOpenGLBatchedMySpecialPolyDataMapper&) = delete;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLBatchedMySpecialPolyDataMapper);

//----------------------------------------------------------------------------
void vtkOpenGLBatchedMySpecialPolyDataMapper::ReplaceShaderColor(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act)
{
  // do GLSL replacements here.
  this->Superclass::ReplaceShaderColor(shaders, ren, act);
}

//----------------------------------------------------------------------------
void vtkOpenGLBatchedMySpecialPolyDataMapper::ReplaceShaderPositionVC(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  // do GLSL replacements here.
  this->Superclass::ReplaceShaderPositionVC(shaders, ren, actor);
}

//------------------------------------------------------------------------------
void vtkOpenGLBatchedMySpecialPolyDataMapper::AppendOneBufferObject(vtkRenderer* ren, vtkActor* act,
  GLBatchElement* glBatchElement, vtkIdType& vertex_offset, std::vector<unsigned char>& colors,
  std::vector<float>& norms)
{
  // get the polydata for current batch element if you want scalars or vectors from it.
  vtkPolyData* poly = glBatchElement->Parent.PolyData;
  (void)poly;
  this->Superclass::AppendOneBufferObject(ren, act, glBatchElement, vertex_offset, colors, norms);
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLMySpecialPolyDataMapperDelegator);

//------------------------------------------------------------------------------
vtkOpenGLMySpecialPolyDataMapperDelegator::vtkOpenGLMySpecialPolyDataMapperDelegator()
{
  if (this->Delegate != nullptr)
  {
    // delete the delegate created by parent class
    this->Delegate = nullptr;
  }
  // create our own.
  this->GLDelegate = vtkOpenGLBatchedMySpecialPolyDataMapper::New();
  this->Delegate = vtk::TakeSmartPointer(this->GLDelegate);
}

//------------------------------------------------------------------------------
vtkOpenGLMySpecialPolyDataMapperDelegator::~vtkOpenGLMySpecialPolyDataMapperDelegator() = default;

//------------------------------------------------------------------------------
void vtkOpenGLMySpecialPolyDataMapperDelegator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkOpenGLMySpecialPolyDataMapperDelegator::ShallowCopy(vtkCompositePolyDataMapper* cpdm)
{
  this->Superclass::ShallowCopy(cpdm);
  // Example: Send the input array to the delegate.
  this->GLDelegate->SetInputArrayToProcess(0, cpdm->GetInputArrayInformation(0));
}

VTK_ABI_NAMESPACE_END
