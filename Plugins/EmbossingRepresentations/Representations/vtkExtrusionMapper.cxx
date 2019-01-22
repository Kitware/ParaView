/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtrusionMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtrusionMapper.h"

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSetAttributes.h"
#include "vtkHardwareSelector.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
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
#include "vtk_glew.h"

// this header have to be included after vtkHardwareSelector
#include "vtkCompositePolyDataMapper2Internal.h"

#include <map>
#include <sstream>
#include <vector>

#include "vtkExtrudeCell_gs.h"

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
    vtkIdType* p = array[typeIdx]->GetPointer();
    for (vtkIdType i = 0; i < nbCells[typeIdx]; i++)
    {
      vtkIdType currentSize = *p++;

      // check for duplicates
      bool duplicates = false;
      for (vtkIdType j = 0; !duplicates && j < currentSize - 1; j++)
      {
        for (vtkIdType k = j + 1; k < currentSize; k++)
        {
          if (p[j] == p[k])
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
      p += currentSize;
    }
  }
}
}

class vtkExtrusionMapperHelper : public vtkCompositeMapperHelper2
{
public:
  static vtkExtrusionMapperHelper* New();
  vtkTypeMacro(vtkExtrusionMapperHelper, vtkCompositeMapperHelper2);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  /**
   * Get data range.
   */
  void GetDataRange(double range[2]);

protected:
  vtkExtrusionMapperHelper();
  ~vtkExtrusionMapperHelper() override = default;

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

  bool GetNeedToRebuildShaders(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

  vtkNew<vtkTextureObject> CellExtrudeTexture;
  vtkNew<vtkOpenGLBufferObject> CellExtrudeBuffer;
  bool NeedRebuild = false;

private:
  vtkExtrusionMapperHelper(const vtkExtrusionMapperHelper&) = delete;
  void operator=(const vtkExtrusionMapperHelper&) = delete;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkExtrusionMapperHelper);

//-----------------------------------------------------------------------------
vtkExtrusionMapperHelper::vtkExtrusionMapperHelper()
{
  this->CellExtrudeBuffer->SetType(vtkOpenGLBufferObject::TextureBuffer);
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapperHelper::ReleaseGraphicsResources(vtkWindow* win)
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
void vtkExtrusionMapperHelper::RenderPieceStart(vtkRenderer* ren, vtkActor* actor)
{
  this->Superclass::RenderPieceStart(ren, actor);

  vtkExtrusionMapper* parent = static_cast<vtkExtrusionMapper*>(this->Parent);

  if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    this->CellExtrudeTexture->Activate();
  }
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapperHelper::RenderPieceFinish(vtkRenderer* ren, vtkActor* actor)
{
  this->Superclass::RenderPieceFinish(ren, actor);

  vtkExtrusionMapper* parent = static_cast<vtkExtrusionMapper*>(this->Parent);

  if (parent->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    this->CellExtrudeTexture->Deactivate();
  }
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapperHelper::BuildBufferObjects(vtkRenderer* ren, vtkActor* act)
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
void vtkExtrusionMapperHelper::GetDataRange(double range[2])
{
  vtkPolyData* input = this->Data.begin()->first;
  vtkDataArray* scalars = this->GetInputArrayToProcess(0, input);
  if (scalars)
  {
    scalars->GetRange(range);
  }
}

//-----------------------------------------------------------------------------
bool vtkExtrusionMapperHelper::GetNeedToRebuildShaders(
  vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act)
{
  bool rebuild = this->NeedRebuild;
  this->NeedRebuild = false;

  return this->Superclass::GetNeedToRebuildShaders(cellBO, ren, act) || rebuild;
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapperHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------
void vtkExtrusionMapperHelper::AppendOneBufferObject(vtkRenderer* ren, vtkActor* act,
  vtkCompositeMapperHelperData* hdata, vtkIdType& voffset, std::vector<unsigned char>& newColors,
  std::vector<float>& newNorms)
{
  vtkExtrusionMapper* parent = static_cast<vtkExtrusionMapper*>(this->Parent);

  if (parent->FieldAssociation != vtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    vtkDataArray* scalars = this->GetInputArrayToProcess(0, this->CurrentInput);
    if (scalars)
    {
      // create "scalar" attribute on vertex buffer
      this->VBOs->AppendDataArray("scalar", scalars, scalars->GetDataType());
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

  this->Superclass::AppendOneBufferObject(ren, act, hdata, voffset, newColors, newNorms);
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapperHelper::ReplaceShaderValues(
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
void vtkExtrusionMapperHelper::SetShaderValues(
  vtkShaderProgram* prog, vtkCompositeMapperHelperData* hdata, size_t primOffset)
{
  this->Superclass::SetShaderValues(prog, hdata, primOffset);

  // update uniforms
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
vtkStandardNewMacro(vtkExtrusionMapper);
vtkSetObjectImplementationMacro(vtkExtrusionMapper, Controller, vtkMultiProcessController);

//-----------------------------------------------------------------------------
vtkExtrusionMapper::vtkExtrusionMapper()
{
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->ResetDataRange();
  this->UserRange[0] = 0.;
  this->UserRange[1] = 1.;
}

//-----------------------------------------------------------------------------
vtkExtrusionMapper::~vtkExtrusionMapper()
{
  if (this->Controller)
  {
    this->Controller->Delete();
    this->Controller = nullptr;
  }
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NormalizeData: " << this->NormalizeData << std::endl;
  os << indent << "ExtrusionFactor: " << this->ExtrusionFactor << std::endl;
  os << indent << "BasisVisibility: " << this->BasisVisibility << std::endl;
  os << indent << "AutoScaling: " << this->AutoScaling << std::endl;
  if (!this->AutoScaling)
  {
    os << indent << "UserRange: " << this->UserRange[0] << ", " << this->UserRange[1] << std::endl;
  }
  os << indent << "BasisVisibility: " << this->BasisVisibility << std::endl;
}

//-----------------------------------------------------------------------------
vtkCompositeMapperHelper2* vtkExtrusionMapper::CreateHelper()
{
  vtkCompositeMapperHelper2* helper = vtkExtrusionMapperHelper::New();

  // copy input array to child
  helper->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
  return helper;
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapper::ComputeBounds()
{
  vtkMTimeType time = this->BoundsMTime.GetMTime();

  this->Superclass::ComputeBounds();

  // if bounds are modified
  if (time < this->BoundsMTime.GetMTime())
  {
    vtkBoundingBox bbox(this->Bounds);
    this->MaxBoundsLength = bbox.GetMaxLength();
    // inflate bounds to handle extrusion in all directions
    bbox.Inflate(this->MaxBoundsLength);
    bbox.GetBounds(this->Bounds);
  }
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapper::SetExtrusionFactor(float factor)
{
  if (this->ExtrusionFactor == factor)
  {
    return;
  }
  for (auto& helper : this->Helpers)
  {
    vtkExtrusionMapperHelper* mapper = static_cast<vtkExtrusionMapperHelper*>(helper.second);
    if (this->ExtrusionFactor == 0.f || factor == 0.f)
    {
      mapper->SetNeedRebuild(true);
    }
  }

  this->ExtrusionFactor = factor;
  this->Modified();
}

// ---------------------------------------------------------------------------
void vtkExtrusionMapper::ResetDataRange()
{
  this->LocalDataRange[0] = VTK_DOUBLE_MAX;
  this->LocalDataRange[1] = VTK_DOUBLE_MIN;
  this->GlobalDataRange[0] = VTK_DOUBLE_MAX;
  this->GlobalDataRange[1] = VTK_DOUBLE_MIN;
}

//----------------------------------------------------------------------------
void vtkExtrusionMapper::InitializeHelpersBeforeRendering(
  vtkRenderer* vtkNotUsed(ren), vtkActor* vtkNotUsed(actor))
{
  // Compute the local range of the data that will be used as extrusion factors
  double range[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  if (!this->NormalizeData == 0)
  {
    return;
  }

  for (auto& helper : this->Helpers)
  {
    double lrange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
    vtkExtrusionMapperHelper* h = dynamic_cast<vtkExtrusionMapperHelper*>(helper.second);
    h->GetDataRange(lrange);
    range[0] = std::min(range[0], lrange[0]);
    range[1] = std::max(range[1], lrange[1]);
  }

  if (range[0] != this->LocalDataRange[0] || range[1] != this->LocalDataRange[1])
  {
    this->GlobalDataRange[0] = range[0];
    this->GlobalDataRange[1] = range[1];
    this->LocalDataRange[0] = range[0];
    this->LocalDataRange[1] = range[1];

    // In parallel, we need to reduce the local ranges to get the global ones.
    if (!this->Controller)
    {
      this->Controller = vtkMultiProcessController::GetGlobalController();
    }
    if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
    {
      this->Controller->AllReduce(&range[0], &this->GlobalDataRange[0], 1, vtkCommunicator::MIN_OP);
      this->Controller->AllReduce(&range[1], &this->GlobalDataRange[1], 1, vtkCommunicator::MAX_OP);
    }
  }
}

//----------------------------------------------------------------------------
void vtkExtrusionMapper::SetInputArrayToProcess(int idx, vtkInformation* inInfo)
{
  this->Superclass::SetInputArrayToProcess(idx, inInfo);

  this->FieldAssociation = inInfo->Get(vtkDataObject::FIELD_ASSOCIATION());
  this->ResetDataRange();
}

//----------------------------------------------------------------------------
void vtkExtrusionMapper::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, int attributeType)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, attributeType);

  this->FieldAssociation = fieldAssociation;
  this->ResetDataRange();
}

//-----------------------------------------------------------------------------
void vtkExtrusionMapper::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);

  this->FieldAssociation = fieldAssociation;
  this->ResetDataRange();
}
