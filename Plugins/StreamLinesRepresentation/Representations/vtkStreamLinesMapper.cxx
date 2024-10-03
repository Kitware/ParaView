// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStreamLinesMapper.h"

#include "vtkActor.h"
#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellLocator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderPass.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLTexture.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkScalarsToColors.h"
#include "vtkShader.h"
#include "vtkShaderProgram.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkTextureObjectVS.h" // a pass through shader
#include "vtkUnsignedCharArray.h"

#include "vtk_glad.h"

#include <algorithm>
#include <vector>

extern const char* vtkStreamLinesBlending_fs;
extern const char* vtkStreamLinesCopy_fs;
extern const char* vtkStreamLines_fs;
extern const char* vtkStreamLines_gs;
extern const char* vtkStreamLines_vs;

//----------------------------------------------------------------------------
namespace
{
template <class T>
void ReleaseVTKGLObject(T*& object, vtkWindow* renWin)
{
  if (object)
  {
    object->ReleaseGraphicsResources(renWin);
    object->Delete();
    object = nullptr;
  }
}

template <class T>
void ReleaseVTKGLObject(T*& object)
{
  if (object)
  {
    object->ReleaseGraphicsResources();
    object->Delete();
    object = nullptr;
  }
}
float s_quadTCoords[8] = { 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f };
float s_quadVerts[12] = { -1.f, -1.f, 0.f, 1.f, -1.f, 0.f, 1.f, 1.f, 0.f, -1.f, 1.f, 0.f };
}

//----------------------------------------------------------------------------
class vtkStreamLinesMapper::Private : public vtkObject
{
public:
  static Private* New();

  void ReleaseGraphicsResources(vtkWindow* renWin)
  {
    ReleaseVTKGLObject(this->VBOs, renWin);
    ReleaseVTKGLObject(this->StreamLineColorBlendProgram, renWin);
    ReleaseVTKGLObject(this->CurrentBuffer, renWin);
    ReleaseVTKGLObject(this->CurrentTexture, renWin);
    ReleaseVTKGLObject(this->CurrentDepthTexture, renWin);
    ReleaseVTKGLObject(this->FrameBuffer, renWin);
    ReleaseVTKGLObject(this->FrameTexture, renWin);
    ReleaseVTKGLObject(this->FrameDepthTexture, renWin);
    ReleaseVTKGLObject(this->StreamLineSegmentProgram, renWin);
    ReleaseVTKGLObject(this->FinalImageBlendProgram, renWin);
  }

  void SetMapper(vtkStreamLinesMapper* mapper) { this->Mapper = mapper; }

  void SetNumberOfParticles(int);

  void SetData(vtkDataSet*, vtkDataArray*, vtkDataArray*);

  // Get the mtime of all renderpasses
  vtkMTimeType GetRenderPassStageMTime(vtkActor* actor);
  // Get whether the "image copy" program needs to be rebuilt, typically done in different stages of
  // a render pass.
  bool GetNeedToRebuidFinalImageCopyShader(vtkActor* actor);
  // Ask the render passes to paste source code in our "image copy" program.
  void ReplaceShaderRenderPass(
    std::string& vss, std::string& gss, std::string& fss, vtkActor* act, bool prePass);
  // Build fbos, textures and shader programs.
  bool PrepareGLBuffers(vtkRenderer*, vtkActor*);
  // Pass 1: Render segment to current buffer FBO
  void DrawParticles(vtkRenderer*, vtkActor*, bool);
  // Pass 2: Blend current and previous frame in the frame buffer FBO
  void BlendStreamlineColor(vtkRenderer*, vtkActor*, bool);
  // Pass 3: Finally draw the FBO onto the screen
  void BlendFinalImage(vtkRenderer*, vtkActor*);
  // Move particles along the flow.
  void UpdateParticles();

protected:
  Private();
  ~Private() override;

  void InitParticle(int);
  bool InterpolateSpeedAndColor(
    std::array<double, 3>& position, std::array<double, 3>& velocity, vtkIdType particleId);

  double Rand(double vmin = 0., double vmax = 1.)
  {
    this->RandomNumberSequence->Next();
    return this->RandomNumberSequence->GetRangeValue(vmin, vmax);
  }

  vtkCellLocator* Locator;
  vtkOpenGLFramebufferObject* CurrentBuffer;
  vtkOpenGLFramebufferObject* FrameBuffer;
  vtkOpenGLShaderCache* ShaderCache;
  vtkOpenGLVertexBufferObjectGroup* VBOs;
  vtkShaderProgram* StreamLineColorBlendProgram;
  vtkShaderProgram* StreamLineSegmentProgram;
  vtkShaderProgram* FinalImageBlendProgram;
  vtkSmartPointer<vtkMinimalStandardRandomSequence> RandomNumberSequence;
  vtkStreamLinesMapper* Mapper;
  vtkTextureObject* CurrentTexture;
  vtkTextureObject* CurrentDepthTexture;
  vtkTextureObject* FrameTexture;
  vtkTextureObject* FrameDepthTexture;
  vtkNew<vtkMatrix4x4> TempMatrix4;
  vtkNew<vtkInformation> LastRenderPassInfo;

  double Bounds[6];
  std::vector<int> ParticlesTTL;
  vtkDataArray* InterpolationArray;
  vtkDataArray* Scalars;
  vtkDataArray* Vectors;
  vtkDataSet* DataSet;
  vtkNew<vtkGenericCell> GenericCell;
  vtkNew<vtkIdList> IdList;
  vtkNew<vtkPoints> Particles;
  vtkSmartPointer<vtkDataArray> InterpolationScalarArray;
  vtkMTimeType ActorMTime;
  vtkMTimeType CameraMTime;
  vtkTimeStamp FinalImageBlendProgramSourceTime;

  bool AreCellScalars;
  bool AreCellVectors;
  bool ClearFlag;
  bool CreateWideLines;

private:
  Private(const Private&) = delete;
  void operator=(const Private&) = delete;
};

vtkStandardNewMacro(vtkStreamLinesMapper::Private);

//----------------------------------------------------------------------------
vtkStreamLinesMapper::Private::Private()
{
  this->Mapper = nullptr;
  this->RandomNumberSequence = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  this->RandomNumberSequence->SetSeed(1);
  this->VBOs = nullptr;
  this->ShaderCache = nullptr;
  this->CurrentBuffer = nullptr;
  this->FrameBuffer = nullptr;
  this->CurrentTexture = nullptr;
  this->CurrentDepthTexture = nullptr;
  this->FrameTexture = nullptr;
  this->FrameDepthTexture = nullptr;
  this->StreamLineSegmentProgram = nullptr;
  this->StreamLineColorBlendProgram = nullptr;
  this->FinalImageBlendProgram = nullptr;
  this->Particles->SetDataTypeToFloat();
  this->InterpolationArray = nullptr;
  this->InterpolationScalarArray = nullptr;
  this->Vectors = nullptr;
  this->Scalars = this->Particles->GetData();
  this->DataSet = nullptr;
  this->ClearFlag = true;
  this->Locator = nullptr;
  this->ActorMTime = 0;
  this->CameraMTime = 0;
  this->AreCellVectors = false;
  this->AreCellScalars = false;
  this->CreateWideLines = false;
}

//----------------------------------------------------------------------------
vtkStreamLinesMapper::Private::~Private()
{
  if (this->InterpolationArray)
  {
    this->InterpolationArray->Delete();
    this->InterpolationArray = nullptr;
  }
  if (this->InterpolationScalarArray)
  {
    this->InterpolationScalarArray->Delete();
    this->InterpolationScalarArray = nullptr;
  }
  if (this->Locator)
  {
    this->Locator->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::Private::SetNumberOfParticles(int nbParticles)
{
  this->Particles->SetNumberOfPoints(nbParticles * 2);
  this->ParticlesTTL.resize(nbParticles, 0);
  if (this->InterpolationScalarArray)
  {
    this->InterpolationScalarArray->Resize(nbParticles * 2);
  }
}

//-----------------------------------------------------------------------------
bool vtkStreamLinesMapper::Private::InterpolateSpeedAndColor(
  std::array<double, 3>& position, std::array<double, 3>& velocity, vtkIdType particleId)
{
  int subId;
  double pcoords[3];
  static double weights[1024];

  vtkIdType cellId = 0;
  if (!this->Locator)
  {
    cellId = this->DataSet->FindCell(position.data(), nullptr, -1, 1e-10, subId, pcoords, weights);
  }
  else
  {
    cellId =
      this->Locator->FindCell(position.data(), 0., this->GenericCell.Get(), pcoords, weights);
  }

  if (cellId < 0)
  {
    return false;
  }

  if (!this->Vectors && !this->Scalars)
  {
    return true;
  }

  this->DataSet->GetCellPoints(cellId, this->IdList.Get());
  if (this->Vectors)
  {
    if (this->AreCellVectors)
    {
      this->Vectors->GetTuple(cellId, velocity.data());
    }
    else
    {
      this->InterpolationArray->InterpolateTuple(0, this->IdList.Get(), this->Vectors, weights);
      this->InterpolationArray->GetTuple(0, velocity.data());
    }
    double speed = vtkMath::Norm(velocity.data());
    if (speed == 0. || vtkMath::IsInf(speed) || vtkMath::IsNan(speed))
    {
      // Null speed area
      return false;
    }
  }

  if (this->Scalars)
  {
    if (this->AreCellScalars)
    {
      this->InterpolationScalarArray->SetTuple(particleId, this->Scalars->GetTuple(cellId));
    }
    else
    {
      this->InterpolationScalarArray->InterpolateTuple(
        particleId, this->IdList.Get(), this->Scalars, weights);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
void vtkStreamLinesMapper::Private::InitParticle(int pid)
{
  bool added = false;
  do
  {
    // Sample a new seed location
    std::array<double, 3> position;
    position[0] = this->Rand(this->Bounds[0], this->Bounds[1]);
    position[1] = this->Rand(this->Bounds[2], this->Bounds[3]);
    position[2] = this->Rand(this->Bounds[4], this->Bounds[5]);
    this->Particles->SetPoint(pid * 2 + 0, position.data());
    this->Particles->SetPoint(pid * 2 + 1, position.data());
    this->ParticlesTTL[pid] = this->Rand(1, this->Mapper->MaxTimeToLive);

    // Check speed at this location
    std::array<double, 3> velocity;
    if (this->InterpolateSpeedAndColor(position, velocity, pid * 2))
    {
      this->InterpolationScalarArray->SetTuple(
        pid * 2 + 1, this->InterpolationScalarArray->GetTuple(pid * 2));
      double speed = vtkMath::Norm(velocity.data());
      // Do not sample in no-speed areas
      added = (speed != 0. && !vtkMath::IsInf(speed) && !vtkMath::IsNan(speed));
    }
  } while (!added);
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::Private::UpdateParticles()
{
  const double dt = this->Mapper->StepLength;

  int nbParticles = static_cast<int>(this->ParticlesTTL.size());

  for (int i = 0; i < nbParticles; ++i)
  {
    this->ParticlesTTL[i]--;
    if (this->ParticlesTTL[i] > 0)
    {
      std::array<double, 3> position;
      this->Particles->GetPoint(i * 2 + 1, position.data());

      // Advance particle position.
      this->Particles->SetPoint(i * 2 + 0, position.data());
      this->InterpolationScalarArray->SetTuple(
        i * 2 + 0, this->InterpolationScalarArray->GetTuple(i * 2 + 1));

      // Move the particle and fetch its color
      std::array<double, 3> velocity;
      if (this->InterpolateSpeedAndColor(position, velocity, i * 2 + 1))
      {
        this->Particles->SetPoint(2 * i + 1, position[0] + dt * velocity[0],
          position[1] + dt * velocity[1], position[2] + dt * velocity[2]);
      }
      else
      {
        this->ParticlesTTL[i] = 0;
      }
    }
    if (this->ParticlesTTL[i] <= 0)
    {
      // Resample dead or out-of-bounds particle
      this->InitParticle(i);
    }
  }
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::Private::DrawParticles(vtkRenderer* ren, vtkActor* actor, bool animate)
{
  vtkOpenGLCamera* cam = vtkOpenGLCamera::SafeDownCast(ren->GetActiveCamera());

  this->ClearFlag = this->ClearFlag || this->Mapper->Alpha == 0. ||
    this->ActorMTime < actor->GetMTime() || this->CameraMTime < cam->GetMTime();

  if (this->ClearFlag && !animate)
  {
    return;
  }

  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  int nbParticles = static_cast<int>(this->ParticlesTTL.size());

  vtkMatrix4x4* wcdc;
  vtkMatrix4x4* wcvc;
  vtkMatrix3x3* norms;
  vtkMatrix4x4* vcdc;
  cam->GetKeyMatrices(ren, wcvc, norms, vcdc, wcdc);
  this->ActorMTime = actor->GetMTime();

  this->CurrentBuffer->SetContext(renWin);
  this->CurrentBuffer->SaveCurrentBindingsAndBuffers();
  this->CurrentBuffer->Bind();
#ifdef VTK_UPDATED_FRAMEBUFFER
  this->CurrentBuffer->AddColorAttachment(0, this->CurrentTexture);
#else
  this->CurrentBuffer->AddColorAttachment(
    this->CurrentBuffer->GetBothMode(), 0, this->CurrentTexture);
#endif
  this->CurrentBuffer->AddDepthAttachment(this->CurrentDepthTexture);
  this->CurrentBuffer->ActivateBuffer(0);
  this->CurrentBuffer->Start(this->CurrentTexture->GetWidth(), this->CurrentTexture->GetHeight());

  this->ShaderCache->ReadyShaderProgram(this->StreamLineSegmentProgram);
  if (this->StreamLineSegmentProgram->IsUniformUsed("MCDCMatrix"))
  {
    actor->ComputeMatrix();
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      static_cast<vtkOpenGLActor*>(actor)->GetKeyMatrices(mcwc, anorms);
      vtkMatrix4x4::Multiply4x4(mcwc, wcdc, this->TempMatrix4.Get());
      this->StreamLineSegmentProgram->SetUniformMatrix("MCDCMatrix", this->TempMatrix4.Get());
    }
    else
    {
      this->StreamLineSegmentProgram->SetUniformMatrix("MCDCMatrix", wcdc);
    }
  }

  bool useScalars = this->Scalars && this->Mapper->GetScalarVisibility();
  double* col = actor->GetProperty()->GetDiffuseColor();
  float color[3];
  color[0] = col[0];
  color[1] = col[1];
  color[2] = col[2];
  this->StreamLineSegmentProgram->SetUniform3f("color", color);
  this->StreamLineSegmentProgram->SetUniformi("scalarVisibility", useScalars);

  if (this->CreateWideLines && this->StreamLineSegmentProgram->IsUniformUsed("lineWidthNVC"))
  {
    int vp[4];
    ostate->vtkglGetIntegerv(GL_VIEWPORT, vp);
    float lineWidth[2];
    lineWidth[0] = 2.0 * actor->GetProperty()->GetLineWidth() / vp[2];
    lineWidth[1] = 2.0 * actor->GetProperty()->GetLineWidth() / vp[3];
    this->StreamLineSegmentProgram->SetUniform2f("lineWidthNVC", lineWidth);
  }

  vtkSmartPointer<vtkUnsignedCharArray> colors = nullptr;
  if (useScalars)
  {
    colors.TakeReference(this->Mapper->GetLookupTable()->MapScalars(this->InterpolationScalarArray,
      this->Mapper->GetColorMode(), this->Mapper->GetArrayComponent()));
  }

  this->VBOs->ClearAllVBOs();
  this->VBOs->ClearAllDataArrays();

  // Create the VBOs
  // Note: we provide dummy colors in case scalars are not visible
  this->VBOs->AppendDataArray("vertexMC", this->Particles->GetData(), VTK_FLOAT);
  this->VBOs->AppendDataArray(
    "scalarColor", colors ? colors : this->Particles->GetData(), VTK_UNSIGNED_CHAR);
  this->VBOs->BuildAllVBOs(ren);

  // Setup the VAO
  vtkNew<vtkOpenGLVertexArrayObject> vao;
  vao->Bind();
  this->VBOs->AddAllAttributesToVAO(this->StreamLineSegmentProgram, vao.Get());

  // Write to depth buffer and color buffer
  vtkOpenGLState::ScopedglEnableDisable depthSaver(ostate, GL_DEPTH_TEST);
  ostate->vtkglEnable(GL_DEPTH_TEST);
  ostate->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
  ostate->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (!this->CreateWideLines)
  {
    glLineWidth(actor->GetProperty()->GetLineWidth());
  }
  glDrawArrays(GL_LINES, 0, nbParticles * 2);
  vtkOpenGLCheckErrorMacro("Failed after rendering");
  vao->Release();

  this->CurrentBuffer->UnBind();
  this->CurrentBuffer->RestorePreviousBindingsAndBuffers();
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::Private::BlendStreamlineColor(
  vtkRenderer* ren, vtkActor* actor, bool animate)
{
  vtkOpenGLCamera* cam = vtkOpenGLCamera::SafeDownCast(ren->GetActiveCamera());

  this->ClearFlag = this->ClearFlag || this->Mapper->Alpha == 0. ||
    this->ActorMTime < actor->GetMTime() || this->CameraMTime < cam->GetMTime();

  if (this->ClearFlag && !animate)
  {
    return;
  }

  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  if (!animate)
  {
    return;
  }
  this->FrameBuffer->SetContext(renWin);
  this->FrameBuffer->SaveCurrentBindingsAndBuffers();
  this->FrameBuffer->Bind();
#ifdef VTK_UPDATED_FRAMEBUFFER
  this->FrameBuffer->AddColorAttachment(0, this->FrameTexture);
#else
  this->FrameBuffer->AddColorAttachment(this->FrameBuffer->GetBothMode(), 0, this->FrameTexture);
#endif
  this->FrameBuffer->AddDepthAttachment(this->FrameDepthTexture);
  this->FrameBuffer->ActivateBuffer(0);
  this->FrameBuffer->Start(this->FrameTexture->GetWidth(), this->FrameTexture->GetHeight());
  // We do not want depth testing at all. Instead, we only want to write to the depth buffer.
  // The call to vtkOpenGLFramebufferObject::Start above disables GL_DEPTH_TEST.
  // OpenGL says that writing to depth buffer is disabled with depth testing turned off.
  ostate->vtkglEnable(GL_DEPTH_TEST);
  vtkOpenGLState::ScopedglDepthFunc depthFuncSaver(ostate);
  ostate->vtkglDepthFunc(GL_ALWAYS);
  if (this->ClearFlag)
  {
    // Clear frame buffer if camera changed
    ostate->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    this->CameraMTime = cam->GetMTime();
    this->ClearFlag = false;
  }

  this->ShaderCache->ReadyShaderProgram(this->StreamLineColorBlendProgram);
  vtkNew<vtkOpenGLVertexArrayObject> vaotb;
  vaotb->Bind();
  this->FrameTexture->Activate();
  this->FrameDepthTexture->Activate();
  this->CurrentTexture->Activate();
  this->CurrentDepthTexture->Activate();
  double alpha =
    1.0 - (1.0 / (this->Mapper->MaxTimeToLive * std::max(0.00001, this->Mapper->Alpha)));
  this->StreamLineColorBlendProgram->SetUniformf("alpha", alpha);
  this->StreamLineColorBlendProgram->SetUniformi("prev", this->FrameTexture->GetTextureUnit());
  this->StreamLineColorBlendProgram->SetUniformi("current", this->CurrentTexture->GetTextureUnit());
  this->StreamLineColorBlendProgram->SetUniformi(
    "prevDepth", this->FrameDepthTexture->GetTextureUnit());
  this->StreamLineColorBlendProgram->SetUniformi(
    "currentDepth", this->CurrentDepthTexture->GetTextureUnit());
  vtkOpenGLRenderUtilities::RenderQuad(
    ::s_quadVerts, ::s_quadTCoords, this->StreamLineColorBlendProgram, vaotb.Get());
  this->CurrentTexture->Deactivate();
  this->CurrentDepthTexture->Deactivate();
  vaotb->Release();

  this->FrameBuffer->UnBind();
  this->FrameBuffer->RestorePreviousBindingsAndBuffers();
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::Private::BlendFinalImage(vtkRenderer* ren, vtkActor* actor)
{
  vtkOpenGLCamera* cam = vtkOpenGLCamera::SafeDownCast(ren->GetActiveCamera());

  this->ClearFlag = this->ClearFlag || this->Mapper->Alpha == 0. ||
    this->ActorMTime < actor->GetMTime() || this->CameraMTime < cam->GetMTime();

  if (this->ClearFlag)
  {
    return;
  }
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  if (this->GetNeedToRebuidFinalImageCopyShader(actor))
  {
    std::string vss = vtkTextureObjectVS;
    std::string fss = vtkStreamLinesCopy_fs;
    std::string gss;
    this->ReplaceShaderRenderPass(vss, gss, fss, actor, /*prePass=*/true);
    this->ReplaceShaderRenderPass(vss, gss, fss, actor, /*prePass=*/false);
    this->FinalImageBlendProgram->UnRegister(this);
    this->FinalImageBlendProgram =
      this->ShaderCache->ReadyShaderProgram(vss.c_str(), fss.c_str(), gss.c_str());
    this->FinalImageBlendProgram->Register(this);
  }
  this->ShaderCache->ReadyShaderProgram(this->FinalImageBlendProgram);
  vtkNew<vtkOpenGLVertexArrayObject> vaot;
  vaot->Bind();
  this->FrameTexture->Activate();
  this->FrameDepthTexture->Activate();
  this->FinalImageBlendProgram->SetUniformi("source", this->FrameTexture->GetTextureUnit());
  this->FinalImageBlendProgram->SetUniformi(
    "depthSource", this->FrameDepthTexture->GetTextureUnit());
  // Handle render pass setup
  vtkInformation* info = actor->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase* rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass* renderPass = static_cast<vtkOpenGLRenderPass*>(rpBase);
      if (!renderPass->SetShaderParameters(this->FinalImageBlendProgram, this->Mapper, actor, vaot))
      {
        vtkErrorMacro(
          "RenderPass::SetShaderParameters failed for renderpass: " << renderPass->GetClassName());
      }
    }
  }
  vtkOpenGLCheckErrorMacro("failed after UpdateShader");
  // Setup blending equation
  int prevBlendParams[4];
  ostate->vtkglGetIntegerv(GL_BLEND_SRC_RGB, &prevBlendParams[0]);
  ostate->vtkglGetIntegerv(GL_BLEND_DST_RGB, &prevBlendParams[1]);
  ostate->vtkglGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendParams[2]);
  ostate->vtkglGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendParams[3]);
  ostate->vtkglEnable(GL_BLEND);
  ostate->vtkglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  // Setup depth test
  ostate->vtkglEnable(GL_DEPTH_TEST);
  vtkOpenGLRenderUtilities::RenderQuad(
    s_quadVerts, s_quadTCoords, this->FinalImageBlendProgram, vaot.Get());

  // Restore blending equation state
  ostate->vtkglBlendFuncSeparate(
    prevBlendParams[0], prevBlendParams[1], prevBlendParams[2], prevBlendParams[3]);

  this->FrameTexture->Deactivate();
  this->FrameDepthTexture->Deactivate();
  vaot->Release();
}

//-----------------------------------------------------------------------------
vtkMTimeType vtkStreamLinesMapper::Private::GetRenderPassStageMTime(vtkActor* actor)
{
  vtkInformation* info = actor->GetPropertyKeys();
  vtkMTimeType renderPassMTime = 0;

  int curRenderPasses = 0;
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    curRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
  }

  int lastRenderPasses = 0;
  if (this->LastRenderPassInfo->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    lastRenderPasses = this->LastRenderPassInfo->Length(vtkOpenGLRenderPass::RenderPasses());
  }
  else // have no last pass
  {
    if (!info) // have no current pass
    {
      return 0; // short circuit
    }
  }

  // Determine the last time a render pass changed stages:
  if (curRenderPasses != lastRenderPasses)
  {
    // Number of passes changed, definitely need to update.
    // Fake the time to force an update:
    renderPassMTime = VTK_MTIME_MAX;
  }
  else
  {
    // Compare the current to the previous render passes:
    for (int i = 0; i < curRenderPasses; ++i)
    {
      vtkObjectBase* curRP = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkObjectBase* lastRP = this->LastRenderPassInfo->Get(vtkOpenGLRenderPass::RenderPasses(), i);

      if (curRP != lastRP)
      {
        // Render passes have changed. Force update:
        renderPassMTime = VTK_MTIME_MAX;
        break;
      }
      else
      {
        // Render passes have not changed -- check MTime.
        vtkOpenGLRenderPass* renderPass = static_cast<vtkOpenGLRenderPass*>(curRP);
        renderPassMTime = std::max(renderPassMTime, renderPass->GetShaderStageMTime());
      }
    }
  }

  // Cache the current set of render passes for next time:
  if (info)
  {
    this->LastRenderPassInfo->CopyEntry(info, vtkOpenGLRenderPass::RenderPasses());
  }
  else
  {
    this->LastRenderPassInfo->Clear();
  }

  return renderPassMTime;
}

//------------------------------------------------------------------------------
bool vtkStreamLinesMapper::Private::GetNeedToRebuidFinalImageCopyShader(vtkActor* actor)
{
  // Have the renderpasses changed?
  vtkMTimeType renderPassMTime = this->GetRenderPassStageMTime(actor);
  return (this->FinalImageBlendProgram == nullptr ||
    this->FinalImageBlendProgramSourceTime < renderPassMTime);
}

//------------------------------------------------------------------------------
void vtkStreamLinesMapper::Private::ReplaceShaderRenderPass(
  std::string& vss, std::string& gss, std::string& fss, vtkActor* act, bool prePass)
{
  vtkInformation* info = act->GetPropertyKeys();
  if (info && info->Has(vtkOpenGLRenderPass::RenderPasses()))
  {
    int numRenderPasses = info->Length(vtkOpenGLRenderPass::RenderPasses());
    for (int i = 0; i < numRenderPasses; ++i)
    {
      vtkObjectBase* rpBase = info->Get(vtkOpenGLRenderPass::RenderPasses(), i);
      vtkOpenGLRenderPass* renderPass = static_cast<vtkOpenGLRenderPass*>(rpBase);
      if (prePass)
      {
        if (!renderPass->PreReplaceShaderValues(vss, gss, fss, this->Mapper, act))
        {
          vtkErrorMacro(
            "vtkOpenGLRenderPass::ReplaceShaderValues failed for " << renderPass->GetClassName());
        }
      }
      else
      {
        if (!renderPass->PostReplaceShaderValues(vss, gss, fss, this->Mapper, act))
        {
          vtkErrorMacro(
            "vtkOpenGLRenderPass::ReplaceShaderValues failed for " << renderPass->GetClassName());
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
bool vtkStreamLinesMapper::Private::PrepareGLBuffers(vtkRenderer* ren, vtkActor* actor)
{
  if (!this->VBOs)
  {
    this->VBOs = vtkOpenGLVertexBufferObjectGroup::New();
  }
  if (!this->CurrentBuffer)
  {
    this->CurrentBuffer = vtkOpenGLFramebufferObject::New();
  }
  if (!this->FrameBuffer)
  {
    this->FrameBuffer = vtkOpenGLFramebufferObject::New();
  }

  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  const int* size = renWin->GetSize();
  unsigned int width = static_cast<unsigned int>(size[0]);
  unsigned int height = static_cast<unsigned int>(size[1]);

  if (!this->CurrentTexture)
  {
    this->CurrentTexture = vtkTextureObject::New();
    this->CurrentTexture->SetContext(renWin);
  }

  if (this->CurrentTexture->GetWidth() != width || this->CurrentTexture->GetHeight() != height)
  {
    this->CurrentTexture->Create2D(width, height, 4, VTK_FLOAT, false);
    this->ClearFlag = true;
  }

  if (!this->CurrentDepthTexture)
  {
    this->CurrentDepthTexture = vtkTextureObject::New();
    this->CurrentDepthTexture->SetContext(renWin);
  }

  if (this->CurrentDepthTexture->GetWidth() != width ||
    this->CurrentDepthTexture->GetHeight() != height)
  {
    this->CurrentDepthTexture->SetWrapS(vtkTextureObject::Repeat);
    this->CurrentDepthTexture->SetWrapT(vtkTextureObject::Repeat);
    this->CurrentDepthTexture->SetMinificationFilter(vtkTextureObject::Nearest);
    this->CurrentDepthTexture->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->CurrentDepthTexture->AllocateDepth(width, height, vtkTextureObject::Fixed24);
  }

  if (!this->FrameTexture)
  {
    this->FrameTexture = vtkTextureObject::New();
    this->FrameTexture->SetContext(renWin);
  }

  if (this->FrameTexture->GetWidth() != width || this->FrameTexture->GetHeight() != height)
  {
    this->FrameTexture->Create2D(width, height, 4, VTK_FLOAT, false);
    this->ClearFlag = true;
  }

  if (!this->FrameDepthTexture)
  {
    this->FrameDepthTexture = vtkTextureObject::New();
    this->FrameDepthTexture->SetContext(renWin);
  }

  if (this->FrameDepthTexture->GetWidth() != width ||
    this->FrameDepthTexture->GetHeight() != height)
  {
    this->FrameDepthTexture->SetWrapS(vtkTextureObject::Repeat);
    this->FrameDepthTexture->SetWrapT(vtkTextureObject::Repeat);
    this->FrameDepthTexture->SetMinificationFilter(vtkTextureObject::Nearest);
    this->FrameDepthTexture->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->FrameDepthTexture->AllocateDepth(width, height, vtkTextureObject::Fixed24);
  }

  if (!this->ShaderCache)
  {
    this->ShaderCache = renWin->GetShaderCache();
  }

  bool prevCreateWideLines = this->CreateWideLines;
  this->CreateWideLines = actor->GetProperty()->GetLineWidth() > 1.0 &&
    actor->GetProperty()->GetLineWidth() > renWin->GetMaximumHardwareLineWidth();

  if (!this->StreamLineSegmentProgram || (prevCreateWideLines != this->CreateWideLines))
  {
    this->ShaderCache->ReleaseCurrentShader();
    if (this->StreamLineSegmentProgram)
    {
      ReleaseVTKGLObject(this->StreamLineSegmentProgram, renWin);
    }
    this->StreamLineSegmentProgram = this->ShaderCache->ReadyShaderProgram(
      vtkStreamLines_vs, vtkStreamLines_fs, this->CreateWideLines ? vtkStreamLines_gs : "");
    this->StreamLineSegmentProgram->Register(this);
  }

  if (!this->StreamLineColorBlendProgram)
  {
    this->StreamLineColorBlendProgram =
      this->ShaderCache->ReadyShaderProgram(vtkTextureObjectVS, vtkStreamLinesBlending_fs, "");
    this->StreamLineColorBlendProgram->Register(this);
  }

  if (!this->FinalImageBlendProgram)
  {
    this->FinalImageBlendProgram =
      this->ShaderCache->ReadyShaderProgram(vtkTextureObjectVS, vtkStreamLinesCopy_fs, "");
    this->FinalImageBlendProgram->Register(this);
  }

  return this->CurrentTexture && this->FrameTexture && this->ShaderCache &&
    this->StreamLineSegmentProgram && this->StreamLineColorBlendProgram &&
    this->FinalImageBlendProgram;
}

namespace
{
bool HaveArray(vtkFieldData* fd, vtkDataArray* inArray)
{
  for (int i = 0; i < fd->GetNumberOfArrays(); i++)
  {
    vtkDataArray* arr = fd->GetArray(i);
    if (arr && arr == inArray)
    {
      return true;
    }
  }
  return false;
}
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::Private::SetData(
  vtkDataSet* inData, vtkDataArray* speedField, vtkDataArray* scalars)
{
  std::size_t nbParticles = this->ParticlesTTL.size();

  if (this->DataSet != inData)
  {
    this->AreCellVectors = false;
    this->AreCellScalars = false;
    inData->GetBounds(this->Bounds);
    this->DataSet = inData;
    this->ClearFlag = true;
    if (this->Locator)
    {
      this->Locator->Delete();
      this->Locator = nullptr;
    }
    if (inData->GetDataObjectType() != VTK_IMAGE_DATA)
    {
      // We need a fast cell locator for any type except imagedata where
      // the FindCell() function is fast enough.
      this->Locator = vtkCellLocator::New();
      this->Locator->SetDataSet(inData);
      this->Locator->BuildLocator();
    }
  }

  if (this->Vectors != speedField)
  {
    this->Vectors = speedField;
    this->ClearFlag = true;
    this->AreCellVectors = ::HaveArray(inData->GetCellData(), speedField);
  }

  if (this->Scalars != scalars)
  {
    if (this->InterpolationScalarArray)
    {
      this->InterpolationScalarArray->Delete();
      this->InterpolationScalarArray = nullptr;
    }
    if (scalars)
    {
      this->InterpolationScalarArray = vtkDataArray::CreateDataArray(scalars->GetDataType());
      this->AreCellScalars = ::HaveArray(inData->GetCellData(), scalars);
    }
    else
    {
      this->InterpolationScalarArray = vtkUnsignedCharArray::New();
    }
    this->InterpolationScalarArray->SetNumberOfComponents(
      scalars ? scalars->GetNumberOfComponents() : 1);
    this->InterpolationScalarArray->SetNumberOfTuples(nbParticles * 2);
    this->Scalars = scalars;
    this->ClearFlag = true;
  }

  if (!this->InterpolationArray ||
    (this->InterpolationArray &&
      this->InterpolationArray->GetDataType() != speedField->GetDataType()))
  {
    if (this->InterpolationArray)
    {
      this->InterpolationArray->Delete();
      this->InterpolationArray = nullptr;
    }
    this->InterpolationArray = vtkDataArray::CreateDataArray(speedField->GetDataType());
    this->InterpolationArray->SetNumberOfComponents(3);
    this->InterpolationArray->SetNumberOfTuples(1);
  }
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkStreamLinesMapper);

//-----------------------------------------------------------------------------
vtkStreamLinesMapper::vtkStreamLinesMapper()
{
  this->Internal = Private::New();
  this->Internal->SetMapper(this);
  this->Animate = true;
  this->Alpha = 0.95;
  this->StepLength = 0.01;
  this->MaxTimeToLive = 600;
  this->NumberOfParticles = 0;
  this->NumberOfAnimationSteps = 1;
  this->AnimationSteps = 0;
  this->SetNumberOfParticles(1000);

  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
  this->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::VECTORS);
}

//-----------------------------------------------------------------------------
vtkStreamLinesMapper::~vtkStreamLinesMapper()
{
  this->Internal->Delete();
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::SetNumberOfParticles(int nbParticles)
{
  if (this->NumberOfParticles == nbParticles)
  {
    return;
  }
  this->NumberOfParticles = nbParticles;
  this->Internal->SetNumberOfParticles(nbParticles);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::SetAnimate(bool animate)
{
  if (this->Animate != animate)
  {
    this->Animate = animate;
    this->AnimationSteps = 0;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::Render(vtkRenderer* ren, vtkActor* actor)
{
  vtkDataSet* inData = vtkDataSet::SafeDownCast(this->GetInput());

  if (!inData || inData->GetNumberOfCells() == 0)
  {
    return;
  }

  vtkDataArray* inScalars =
    this->GetInputArrayToProcess(0, 0, this->GetExecutive()->GetInputInformation());

  vtkDataArray* inVectors =
    this->GetInputArrayToProcess(1, 0, this->GetExecutive()->GetInputInformation());

  if (!inVectors || inVectors->GetNumberOfComponents() != 3)
  {
    vtkDebugMacro(<< "No speed field vector to process!");
    return;
  }

  double magRange[2];
  inVectors->GetRange(magRange, -1);
  const double& minSpeed = magRange[0];
  const double& maxSpeed = magRange[1];
  const bool velocityIsNotValid = (minSpeed == 0. && maxSpeed == 0.) ||
    (vtkMath::IsInf(minSpeed) && vtkMath::IsInf(maxSpeed)) ||
    (vtkMath::IsNan(minSpeed) && vtkMath::IsNan(maxSpeed));

  if (velocityIsNotValid)
  {
    vtkDebugMacro(<< "Speed field vector is zero or not valid!");
    return;
  }

  // Set processing dataset and arrays
  this->Internal->SetData(inData, inVectors, inScalars);

  if (!this->Internal->PrepareGLBuffers(ren, actor))
  {
    return;
  }
  bool animate = true;
  for (int i = 0; i < this->NumberOfAnimationSteps && animate; i++)
  {
    animate = this->Animate &&
      (this->NumberOfAnimationSteps == 1 ||
        (this->NumberOfAnimationSteps > 1 && this->AnimationSteps < this->NumberOfAnimationSteps));
    if (animate && !actor->IsRenderingTranslucentPolygonalGeometry())
    {
      // Move particles
      this->Internal->UpdateParticles();
      if (this->NumberOfAnimationSteps > 1)
      {
        this->AnimationSteps++;
      }
    }

    if (!actor->IsRenderingTranslucentPolygonalGeometry())
    {
      this->Internal->DrawParticles(ren, actor, animate);
      this->Internal->BlendStreamlineColor(ren, actor, animate);
    }
    else
    {
      this->Internal->BlendFinalImage(ren, actor);
    }
  }
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::ReleaseGraphicsResources(vtkWindow* renWin)
{
  this->Internal->ReleaseGraphicsResources(renWin);
}

//----------------------------------------------------------------------------
int vtkStreamLinesMapper::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkStreamLinesMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Alpha : " << this->Alpha << endl;
  os << indent << "StepLength : " << this->StepLength << endl;
  os << indent << "NumberOfParticles: " << this->NumberOfParticles << endl;
  os << indent << "MaxTimeToLive: " << this->MaxTimeToLive << endl;
}
