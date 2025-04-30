// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLExtrusionMapperDelegator.h"
#include "vtkBumpMapMapper.h"
#include "vtkCellArrayIterator.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCompositePolyDataMapperDelegator.h"
#include "vtkExtrudeCell_gs.h"
#include "vtkExtrusionMapper.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBatchedPolyDataMapper.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

#include <array>

VTK_ABI_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
namespace
{
void GetTrianglesFromPolyData(
  vtkPolyData* pd, vtkDataArray* data, std::vector<float>& triangleArray)
{
  if (!pd)
  {
    return;
  }

  // loop on polys and strips to extract triangles from cells
  // create [N-2] values per cell (N number of points on cell)
  vtkCellArray* array[2] = { pd->GetPolys(), pd->GetStrips() };
  vtkIdType nbCells[2] = { pd->GetNumberOfPolys(), pd->GetNumberOfStrips() };
  triangleArray.reserve(
    array[0]->GetSize() - 3 * nbCells[0] + array[1]->GetSize() - 3 * nbCells[1]);

  for (int typeIdx = 0; typeIdx < 2; typeIdx++)
  {
    auto cellIter = vtk::TakeSmartPointer(array[typeIdx]->NewIterator());
    cellIter->GoToFirstCell();
    for (vtkIdType i = 0; i < nbCells[typeIdx]; i++)
    {
      vtkIdList* cell = cellIter->GetCurrentCell();
      vtkIdType currentSize = cell->GetNumberOfIds();

      // check for duplicates
      bool duplicates = false;
      for (vtkIdType j = 0; !duplicates && j < currentSize - 1; j++)
      {
        for (vtkIdType k = j + 1; k < currentSize; k++)
        {
          if (cell->GetId(j) == cell->GetId(k))
          {
            duplicates = true;
            break;
          }
        }
      }

      if (!duplicates)
      {
        triangleArray.insert(
          triangleArray.end(), currentSize - 2, static_cast<float>(data->GetComponent(i, 0)));
      }
      cellIter->GoToNextCell();
    }
  }
}
}

//----------------------------------------------------------------------------
/**
 * The vtkOpenGLBatchedExtrusionMapper inherits indirectly from vtkOpenGLBatchedPolyDataMapper
 * and contains most of the rendering code specific to the vtkExtrusionMapper.
 */
class vtkOpenGLBatchedExtrusionMapper : public vtkOpenGLBatchedPolyDataMapper
{
public:
  static vtkOpenGLBatchedExtrusionMapper* New();
  vtkTypeMacro(vtkOpenGLBatchedExtrusionMapper, vtkOpenGLBatchedPolyDataMapper);

  /**
   * Rebuild shader if extrusion factor change from (or to) zero.
   */
  vtkSetMacro(NeedRebuild, bool);

  /**
   * Override to release texture
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * Override to bind/unbind the texture
   */
  void RenderPieceStart(vtkRenderer* ren, vtkActor* actor) override;
  void RenderPieceFinish(vtkRenderer* ren, vtkActor* actor) override;

  /**
   * Update texture
   */
  void BuildBufferObjects(vtkRenderer* ren, vtkActor* act) override;

  void GetDataRange(double range[2]);

protected:
  vtkOpenGLBatchedExtrusionMapper();
  ~vtkOpenGLBatchedExtrusionMapper() override = default;

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

  bool GetNeedToRebuildShaders(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

  vtkNew<vtkTextureObject> CellExtrudeTexture;
  vtkNew<vtkOpenGLBufferObject> CellExtrudeBuffer;
  bool NeedRebuild = false;

private:
  vtkOpenGLBatchedExtrusionMapper(const vtkOpenGLBatchedExtrusionMapper&) = delete;
  void operator=(const vtkOpenGLBatchedExtrusionMapper&) = delete;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLBatchedExtrusionMapper);

//-------------------------------------------------------------------------
vtkOpenGLBatchedExtrusionMapper::vtkOpenGLBatchedExtrusionMapper()
{
  this->CellExtrudeBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Superclass::ReleaseGraphicsResources(win);

  if (this->CellExtrudeTexture)
  {
    this->CellExtrudeTexture->ReleaseGraphicsResources(win);
  }
  if (this->CellExtrudeBuffer)
  {
    this->CellExtrudeBuffer->ReleaseGraphicsResources();
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::RenderPieceStart(vtkRenderer* ren, vtkActor* actor)
{
  this->Superclass::RenderPieceStart(ren, actor);

  vtkExtrusionMapper* parent = static_cast<vtkExtrusionMapper*>(this->Parent);

  if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    this->CellExtrudeTexture->Activate();
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::RenderPieceFinish(vtkRenderer* ren, vtkActor* actor)
{
  this->Superclass::RenderPieceFinish(ren, actor);

  vtkExtrusionMapper* parent = static_cast<vtkExtrusionMapper*>(this->Parent);

  if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    this->CellExtrudeTexture->Deactivate();
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::BuildBufferObjects(vtkRenderer* ren, vtkActor* act)
{
  this->Superclass::BuildBufferObjects(ren, act);

  vtkExtrusionMapper* parent = static_cast<vtkExtrusionMapper*>(this->Parent);

  // if we have cell data, we construct a float texture
  if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    this->CellExtrudeTexture->SetContext(static_cast<vtkOpenGLRenderWindow*>(ren->GetVTKWindow()));

    std::vector<float> triangleArray;
    ::GetTrianglesFromPolyData(
      this->CurrentInput, this->GetInputArrayToProcess(0, this->CurrentInput), triangleArray);

    // load data to float texture
    this->CellExtrudeBuffer->Upload(triangleArray, vtkOpenGLBufferObject::TextureBuffer);
    this->CellExtrudeTexture->CreateTextureBuffer(
      static_cast<unsigned int>(triangleArray.size()), 1, VTK_FLOAT, this->CellExtrudeBuffer);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::GetDataRange(double range[2])
{
  vtkPolyData* input = this->VTKPolyDataToGLBatchElement.begin()->second->Parent.PolyData;
  vtkDataArray* scalars = this->GetInputArrayToProcess(0, input);
  if (scalars)
  {
    scalars->GetRange(range);
  }
}

//-------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::AppendOneBufferObject(vtkRenderer* ren, vtkActor* act,
  GLBatchElement* glBatchElement, vtkIdType& vertex_offset, std::vector<unsigned char>& colors,
  std::vector<float>& norms)
{
  vtkExtrusionMapper* parent = static_cast<vtkExtrusionMapper*>(this->Parent);

  if (parent->FieldAssociation != vtkDataObject::FIELD_ASSOCIATION_CELLS)
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

    vtkDataArray* normals = this->CurrentInput->GetPointData()->GetNormals();
    vtkNew<vtkPolyDataNormals> normalsFilter;
    if (!normals)
    {
      normalsFilter->SetInputDataObject(this->CurrentInput);
      normalsFilter->Update();
      normals = normalsFilter->GetOutput()->GetPointData()->GetNormals();
    }
    this->VBOs->AppendDataArray("normals", normals, normals->GetDataType());
  }
  this->Superclass::AppendOneBufferObject(ren, act, glBatchElement, vertex_offset, colors, norms);
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::ReplaceShaderValues(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{

  vtkExtrusionMapper* parent = static_cast<vtkExtrusionMapper*>(this->Parent);

  if (parent->GetExtrusionFactor() != 0.f)
  {
    std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
    std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

    if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
      vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Dec",
        "//VTK::PositionVC::Dec\n" // other declarations will be done by the superclass
        "out vec4 vertexMCVSOutput;\n");

      vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Impl",
        "//VTK::PositionVC::Impl\n" // other declarations will be done by the superclass
        "  vertexMCVSOutput = vertexMC;\n");

      shaders[vtkShader::Geometry]->SetSource(vtkExtrudeCell_gs);

      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec", "in vec3 normalVCGSOutput;\n");
      vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl", "");
    }
    else
    {
      vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Dec",
        "//VTK::PositionVC::Dec\n" // other declarations will be done by the superclass
        "uniform vec2 scalarRange;\n"
        "uniform float extrusionFactor;\n"
        "uniform int normalizeData;\n"
        "in float scalar;\n"
        "in vec3 normals;\n");

      vtkShaderProgram::Substitute(
        VSSource, "//VTK::PositionVC::Dec", "out vec4 vertexVCVSOutput;");

      vtkShaderProgram::Substitute(VSSource, "//VTK::Camera::Dec",
        "uniform mat4 MCDCMatrix;\n"
        "uniform mat4 MCVCMatrix;\n");

      vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Impl",
        "float factor = scalar * extrusionFactor;\n"
        "  if (normalizeData != 0)\n"
        "    factor = extrusionFactor * clamp((scalar-scalarRange.x) / "
        "(scalarRange.y-scalarRange.x), 0.0, 1.0);\n"
        "  vec4 dirMC = inverse(MCVCMatrix)*normalize(MCVCMatrix*vec4(normals, 0.0));\n"
        "  vec4 newPosMC = vertexMC + factor*dirMC;\n"
        "  vertexVCVSOutput = MCVCMatrix * newPosMC;\n"
        "  gl_Position = MCDCMatrix * newPosMC;\n");
    }

    shaders[vtkShader::Vertex]->SetSource(VSSource);
    shaders[vtkShader::Fragment]->SetSource(FSSource);
  }

  this->Superclass::ReplaceShaderValues(shaders, ren, actor);
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::SetShaderValues(
  vtkShaderProgram* prog, GLBatchElement* glBatchElement, size_t primOffset)
{
  this->Superclass::SetShaderValues(prog, glBatchElement, primOffset); // update uniforms
  vtkExtrusionMapper* parent = static_cast<vtkExtrusionMapper*>(this->Parent);

  // scale factor to [-MaxBoundsLength ; MaxBoundsLength]
  double factor = (parent->GetExtrusionFactor() * 0.01) * parent->MaxBoundsLength;

  prog->SetUniformf("extrusionFactor", factor);
  prog->SetUniformi("basisVisibility", parent->BasisVisibility);
  prog->SetUniformi("normalizeData", parent->GetNormalizeData() ? 1 : 0);

  float scalarRange[2];
  scalarRange[0] = parent->UserRange[0];
  scalarRange[1] = parent->UserRange[1];

  if (parent->AutoScaling && parent->GetNormalizeData())
  {
    vtkDataArray* scalars = this->GetInputArrayToProcess(0, this->CurrentInput);

    if (scalars && parent->GlobalDataRange[0] == VTK_DOUBLE_MAX)
    {
      scalars->GetRange(parent->GlobalDataRange);
    }
    scalarRange[0] = parent->GlobalDataRange[0];
    scalarRange[1] = parent->GlobalDataRange[1];
  }
  prog->SetUniform2f("scalarRange", scalarRange);

  if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS &&
    prog->IsUniformUsed("textureExtrude"))
  {
    int tunit = this->CellExtrudeTexture->GetTextureUnit();
    prog->SetUniformi("textureExtrude", tunit);
  }
}

//-----------------------------------------------------------------------------
bool vtkOpenGLBatchedExtrusionMapper::GetNeedToRebuildShaders(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act)
{
  const bool rebuild = this->NeedRebuild;
  this->NeedRebuild = false;
  return this->Superclass::GetNeedToRebuildShaders(cellBO, ren, act) || rebuild;
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLExtrusionMapperDelegator);

//------------------------------------------------------------------------------
vtkOpenGLExtrusionMapperDelegator::vtkOpenGLExtrusionMapperDelegator()
{
  if (this->Delegate != nullptr)
  {
    // delete the delegate created by parent class
    this->Delegate = nullptr;
  }
  // create our own.
  this->GLDelegate = vtkOpenGLBatchedExtrusionMapper::New();
  this->Delegate = vtk::TakeSmartPointer(this->GLDelegate);
}

//------------------------------------------------------------------------------
vtkOpenGLExtrusionMapperDelegator::~vtkOpenGLExtrusionMapperDelegator() = default;

//------------------------------------------------------------------------------
void vtkOpenGLExtrusionMapperDelegator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOpenGLExtrusionMapperDelegator::ShallowCopy(vtkCompositePolyDataMapper* cpdm)
{
  this->Superclass::ShallowCopy(cpdm);
  // copy input array to delegate
  this->GLDelegate->SetInputArrayToProcess(0, cpdm->GetInputArrayInformation(0));
}

//------------------------------------------------------------------------------
void vtkOpenGLExtrusionMapperDelegator::GetDataRange(double range[2])
{
  auto glExtrusionMapper = static_cast<vtkOpenGLBatchedExtrusionMapper*>(this->GLDelegate);
  glExtrusionMapper->GetDataRange(range);
}

//------------------------------------------------------------------------------
void vtkOpenGLExtrusionMapperDelegator::SetNeedRebuild(bool value)
{
  auto glExtrusionMapper = static_cast<vtkOpenGLBatchedExtrusionMapper*>(this->GLDelegate);
  glExtrusionMapper->SetNeedRebuild(value);
}

VTK_ABI_NAMESPACE_END
