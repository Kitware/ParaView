// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLExtrusionMapperDelegator.h"
#include "vtkCellArrayIterator.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCompositePolyDataMapperDelegator.h"
#include "vtkComputeTriangleNormals_gs.h"
#include "vtkExtrudeCell_gs.h"
#include "vtkExtrusionMapper.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLBatchedPolyDataMapper.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"

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
  triangleArray.reserve(array[0]->GetNumberOfConnectivityIds() - 3 * nbCells[0] +
    array[1]->GetNumberOfConnectivityIds() - 3 * nbCells[1]);

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
  enum ExtrusionDataType
  {
    None = 0,
    Scalar = 1,
    Vector3 = 3
  };

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

private:
  vtkOpenGLBatchedExtrusionMapper(const vtkOpenGLBatchedExtrusionMapper&) = delete;
  void operator=(const vtkOpenGLBatchedExtrusionMapper&) = delete;

  /**
   * Create a GPU vertex buffer object for the specified data array extrusionData.
   * This function first checks if the given data is valid:
   * - The data type can not be ID type.
   * - The number of components has to be 1 (scalars) or 3 (vector 3).
   */
  void CreateExtrusionAttribtutesVBO(vtkDataArray* extrusionData);

  /**
   * Generate the vertex, geometry and fragment shader code to handle extrusion for point scalars
   * input data.
   */
  void GeneratePointScalarsExtrusionShaders(std::map<vtkShader::Type, vtkShader*>& shaders);

  /**
   * Generate the vertex, geometry and fragment shader code to handle extrusion for point
   * vectors input data.
   */
  void GeneratePointVector3ExtrusionShaders(std::map<vtkShader::Type, vtkShader*>& shaders);

  /**
   * Generate the vertex, geometry and fragment shader code to handle extrusion for cell
   * scalars input data.
   */
  void GenerateCellScalarsExtrusionShaders(std::map<vtkShader::Type, vtkShader*>& shaders);

  ExtrusionDataType ExtrusionVBODataType;
  vtkNew<vtkTextureObject> CellExtrudeTexture;
  vtkNew<vtkOpenGLBufferObject> CellExtrudeBuffer;
  bool NeedRebuild = false;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLBatchedExtrusionMapper);

//-------------------------------------------------------------------------
vtkOpenGLBatchedExtrusionMapper::vtkOpenGLBatchedExtrusionMapper()
{
  this->ExtrusionVBODataType = ExtrusionDataType::None;
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

  vtkExtrusionMapper* parent = vtkExtrusionMapper::SafeDownCast(this->Parent);

  if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    this->CellExtrudeTexture->Activate();
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::RenderPieceFinish(vtkRenderer* ren, vtkActor* actor)
{
  this->Superclass::RenderPieceFinish(ren, actor);

  vtkExtrusionMapper* parent = vtkExtrusionMapper::SafeDownCast(this->Parent);

  if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    this->CellExtrudeTexture->Deactivate();
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::BuildBufferObjects(vtkRenderer* ren, vtkActor* act)
{
  this->Superclass::BuildBufferObjects(ren, act);

  vtkExtrusionMapper* parent = vtkExtrusionMapper::SafeDownCast(this->Parent);

  // if we have cell data, we construct a float texture
  if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    this->ExtrusionVBODataType = ExtrusionDataType::Scalar;

    this->CellExtrudeTexture->SetContext(vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow()));

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
void vtkOpenGLBatchedExtrusionMapper::CreateExtrusionAttribtutesVBO(vtkDataArray* extrusionData)
{
  if (extrusionData->GetDataType() == VTK_ID_TYPE)
  {
    vtkErrorMacro(<< "Data type selected for extrusion is currently ID type which is not "
                     "supported (64bit type)");
    this->ExtrusionVBODataType = ExtrusionDataType::None;
    return;
  }

  int numberOfComponents = extrusionData->GetNumberOfComponents();
  this->ExtrusionVBODataType = static_cast<ExtrusionDataType>(numberOfComponents);
  std::string arrayName;
  switch (this->ExtrusionVBODataType)
  {
    case ExtrusionDataType::Scalar:
      arrayName = "displacementScalar";
      break;
    case ExtrusionDataType::Vector3:
      arrayName = "displacementVector";
      break;
    default:
      vtkLogF(ERROR,
        "Unsupported data type for extrusion! Extrusion surface currently supports scalars or "
        "vector3, got %i components element.",
        numberOfComponents);
      return;
  }

  this->VBOs->AppendDataArray(arrayName.c_str(), extrusionData, extrusionData->GetDataType());
}

//-------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::AppendOneBufferObject(vtkRenderer* ren, vtkActor* act,
  GLBatchElement* glBatchElement, vtkIdType& vertex_offset, std::vector<unsigned char>& colors,
  std::vector<float>& norms)
{
  vtkExtrusionMapper* parent = vtkExtrusionMapper::SafeDownCast(this->Parent);

  if (parent->FieldAssociation != vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    vtkDataArray* extrusionDataArray = this->GetInputArrayToProcess(0, this->CurrentInput);

    if (extrusionDataArray)
    {
      this->CreateExtrusionAttribtutesVBO(extrusionDataArray);
    }

    // Add or create normals
    vtkDataArray* normals = this->CurrentInput->GetPointData()->GetNormals();
    vtkNew<vtkPolyDataNormals> normalsFilter;
    if (!normals)
    {
      normalsFilter->SetInputDataObject(this->CurrentInput);
      normalsFilter->Update();
      normals = normalsFilter->GetOutput()->GetPointData()->GetNormals();
    }
    this->VBOs->AppendDataArray("extrusionNormalsMC", normals, normals->GetDataType());
  }
  this->Superclass::AppendOneBufferObject(ren, act, glBatchElement, vertex_offset, colors, norms);
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::ReplaceShaderValues(
  std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor)
{
  vtkExtrusionMapper* parent = vtkExtrusionMapper::SafeDownCast(this->Parent);

  if (parent->GetExtrusionFactor() != 0.f && this->ExtrusionVBODataType != ExtrusionDataType::None)
  {
    if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
      this->GenerateCellScalarsExtrusionShaders(shaders);
    }
    else
    {
      switch (this->ExtrusionVBODataType)
      {
        case ExtrusionDataType::None:
          vtkLogF(ERROR,
            "Input data type has not been set correctly, unable to generate extrusion shader "
            "code!");
          break;
        case ExtrusionDataType::Scalar:
          this->GeneratePointScalarsExtrusionShaders(shaders);
          break;
        case ExtrusionDataType::Vector3:
          this->GeneratePointVector3ExtrusionShaders(shaders);
          break;
      }
    }
  }

  this->Superclass::ReplaceShaderValues(shaders, ren, actor);
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::GeneratePointScalarsExtrusionShaders(
  std::map<vtkShader::Type, vtkShader*>& shaders)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Dec",
    // other declarations will be done by the superclass, hence keeping the key here
    R"(
  //VTK::PositionVC::Dec
  uniform float extrusionFactor;
  uniform vec3 vertexScaleMC;
  uniform vec2 scalarRange;
  uniform int normalizeData;

  in float displacementScalar;
  in vec3 extrusionNormalsMC;

  out vec4 vertexMCVSOutput;
  out vec3 vertexMCVSUnscaled;)");

  vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Impl",
    R"(
  float factor = displacementScalar * extrusionFactor;
  if (normalizeData != 0)
  {
    factor = extrusionFactor * clamp((displacementScalar - scalarRange.x) / (scalarRange.y - scalarRange.x), 0.0, 1.0);
  }
  vec4 dirMC = inverse(MCVCMatrix) * normalize(MCVCMatrix*vec4(extrusionNormalsMC, 0.0));
  vec4 newPosMC = vertexMC + factor * dirMC;
  vertexMCVSOutput = newPosMC;
  vertexMCVSUnscaled = newPosMC.xyz / vertexScaleMC;
  vertexVCVSOutput = MCVCMatrix * newPosMC;
  gl_Position = MCDCMatrix * newPosMC;)");

  // Geometry shader
  if (vtkExtrusionMapper::SafeDownCast(this->Parent)->RecalculateNormals)
  {
    GSSource = vtkComputeTriangleNormals_gs;

    // Fragment shader
    vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec", "in vec3 normalVCGSOutput;\n");
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::GeneratePointVector3ExtrusionShaders(
  std::map<vtkShader::Type, vtkShader*>& shaders)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkShaderProgram::Substitute(VSSource, "//VTK::Camera::Dec",
    R"(
  uniform mat4 MCDCMatrix;
  uniform mat4 MCVCMatrix;)");

  vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Dec",
    R"(
  uniform float extrusionFactor;
  uniform vec3 vertexScaleMC;
  uniform vec2 scalarRange;
  uniform int normalizeData;

  in vec3 displacementVector;
  in vec3 extrusionNormalsMC;

  out vec3 vertexMCVSUnscaled;
  out vec4 vertexMCVSOutput;
  out vec4 vertexVCVSOutput;)");
  vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Impl",
    R"(
  vec3 normalizationFactor = vec3(1.0); // Make default 1 in case no normalization is needed.
  if (normalizeData != 0)
  {
    normalizationFactor = (displacementVector - scalarRange.x) / (scalarRange.y - scalarRange.x);
    normalizationFactor = clamp(normalizationFactor, 0.0, 1.0);
  }
  vec4 vertexDisplacementMC = vec4(displacementVector * vertexScaleMC * normalizationFactor * extrusionFactor, 0.0);
  vec4 newPosMC = vertexMC + vertexDisplacementMC;
  vertexMCVSUnscaled = newPosMC.xyz / vertexScaleMC;
  vertexMCVSOutput = newPosMC;
  vertexVCVSOutput = MCVCMatrix * vertexMCVSOutput;
  gl_Position = MCDCMatrix * vertexMCVSOutput;)");

  // If triangles are drawn, we need to recalculate their normals, hence attaching the geometry
  // shader.
  if (vtkExtrusionMapper::SafeDownCast(this->Parent)->RecalculateNormals &&
    (this->LastBoundBO->PrimitiveType == vtkOpenGLPolyDataMapper::PrimitiveTris ||
      this->LastBoundBO->PrimitiveType == vtkOpenGLPolyDataMapper::PrimitiveTriStrips))
  {
    GSSource = vtkComputeTriangleNormals_gs;

    vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec", "in vec3 normalVCGSOutput;\n");
  }

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::GenerateCellScalarsExtrusionShaders(
  std::map<vtkShader::Type, vtkShader*>& shaders)
{
  std::string VSSource = shaders[vtkShader::Vertex]->GetSource();
  std::string GSSource = shaders[vtkShader::Geometry]->GetSource();
  std::string FSSource = shaders[vtkShader::Fragment]->GetSource();

  vtkShaderProgram::Substitute(VSSource, "//VTK::Camera::Dec",
    R"(
  uniform mat4 MCDCMatrix;
  uniform mat4 MCVCMatrix;)");

  vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Dec",
    "//VTK::PositionVC::Dec\n" // other declarations will be done by the superclass
    "out vec4 vertexMCVSOutput;\n");

  vtkShaderProgram::Substitute(VSSource, "//VTK::PositionVC::Impl",
    "//VTK::PositionVC::Impl\n" // other declarations will be done by the superclass
    "  vertexMCVSOutput = vertexMC;\n");

  GSSource = vtkExtrudeCell_gs;

  vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Dec", "in vec3 normalVCGSOutput;\n");
  vtkShaderProgram::Substitute(FSSource, "//VTK::Normal::Impl", "");

  shaders[vtkShader::Vertex]->SetSource(VSSource);
  shaders[vtkShader::Geometry]->SetSource(GSSource);
  shaders[vtkShader::Fragment]->SetSource(FSSource);
}

//-----------------------------------------------------------------------------
void vtkOpenGLBatchedExtrusionMapper::SetShaderValues(
  vtkShaderProgram* prog, GLBatchElement* glBatchElement, size_t primOffset)
{
  this->Superclass::SetShaderValues(prog, glBatchElement, primOffset); // update uniforms
  vtkExtrusionMapper* parent = vtkExtrusionMapper::SafeDownCast(this->Parent);

  // Component wise scale, used to scale the displacement before adding the displacement to the
  // vertices.
  const std::vector<double>& scale = this->VBOs->GetVBO("vertexMC")->GetScale();
  prog->SetUniform3f("vertexScaleMC", scale.data());

  double factor = parent->GetExtrusionFactor();
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
  auto glExtrusionMapper = vtkOpenGLBatchedExtrusionMapper::SafeDownCast(this->GLDelegate);
  glExtrusionMapper->GetDataRange(range);
}

//------------------------------------------------------------------------------
void vtkOpenGLExtrusionMapperDelegator::SetNeedRebuild(bool value)
{
  auto glExtrusionMapper = vtkOpenGLBatchedExtrusionMapper::SafeDownCast(this->GLDelegate);
  glExtrusionMapper->SetNeedRebuild(value);
}

VTK_ABI_NAMESPACE_END
