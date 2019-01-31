/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamLinesMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtk_glew.h"

#include <algorithm>
#include <vector>

extern const char* vtkStreamLinesBlending_fs;
extern const char* vtkStreamLinesCopy_fs;
extern const char* vtkStreamLines_fs;
extern const char* vtkStreamLines_gs;
extern const char* vtkStreamLines_vs;

//----------------------------------------------------------------------------
#define RELEASE_VTKGL_OBJECT(_x)                                                                   \
  if (_x)                                                                                          \
  {                                                                                                \
    _x->ReleaseGraphicsResources(renWin);                                                          \
    _x->Delete();                                                                                  \
    _x = 0;                                                                                        \
  }
#define RELEASE_VTKGL_OBJECT2(_x)                                                                  \
  if (_x)                                                                                          \
  {                                                                                                \
    _x->ReleaseGraphicsResources();                                                                \
    _x->Delete();                                                                                  \
    _x = 0;                                                                                        \
  }

//----------------------------------------------------------------------------
class vtkStreamLinesMapper::Private : public vtkObject
{
public:
  static Private* New();

  void ReleaseGraphicsResources(vtkWindow* renWin)
  {
    RELEASE_VTKGL_OBJECT(this->VBOs);
    RELEASE_VTKGL_OBJECT(this->BlendingProgram);
    RELEASE_VTKGL_OBJECT(this->CurrentBuffer);
    RELEASE_VTKGL_OBJECT(this->CurrentTexture);
    RELEASE_VTKGL_OBJECT(this->FrameBuffer);
    RELEASE_VTKGL_OBJECT(this->FrameTexture);
    RELEASE_VTKGL_OBJECT(this->Program);
    RELEASE_VTKGL_OBJECT(this->TextureProgram);
    RELEASE_VTKGL_OBJECT2(this->IndexBufferObject);
  }

  void SetMapper(vtkStreamLinesMapper* mapper) { this->Mapper = mapper; }

  void SetNumberOfParticles(int);

  void SetData(vtkDataSet*, vtkDataArray*, vtkDataArray*);

  void DrawParticles(vtkRenderer*, vtkActor*, bool);

  void UpdateParticles();

protected:
  Private();
  ~Private() override;

  void InitParticle(int);
  bool PrepareGLBuffers(vtkRenderer*, vtkActor*);
  bool InterpolateSpeedAndColor(double[3], double[3], vtkIdType);

  inline double Rand(double vmin = 0., double vmax = 1.)
  {
    this->RandomNumberSequence->Next();
    return this->RandomNumberSequence->GetRangeValue(vmin, vmax);
  }

  vtkCellLocator* Locator;
  vtkOpenGLBufferObject* IndexBufferObject;
  vtkOpenGLFramebufferObject* CurrentBuffer;
  vtkOpenGLFramebufferObject* FrameBuffer;
  vtkOpenGLShaderCache* ShaderCache;
  vtkOpenGLVertexBufferObjectGroup* VBOs;
  vtkShaderProgram* BlendingProgram;
  vtkShaderProgram* Program;
  vtkShaderProgram* TextureProgram;
  vtkSmartPointer<vtkMinimalStandardRandomSequence> RandomNumberSequence;
  vtkStreamLinesMapper* Mapper;
  vtkTextureObject* CurrentTexture;
  vtkTextureObject* FrameTexture;
  vtkNew<vtkMatrix4x4> TempMatrix4;

  double Bounds[6];
  std::vector<int> Indices;
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

  bool AreCellScalars;
  bool AreCellVectors;
  bool ClearFlag;
  bool RebuildBufferObjects;
  bool CreateWideLines;

private:
  Private(const Private&) = delete;
  void operator=(const Private&) = delete;
};

vtkStandardNewMacro(vtkStreamLinesMapper::Private)

  //----------------------------------------------------------------------------
  vtkStreamLinesMapper::Private::Private()
{
  this->Mapper = 0;
  this->RandomNumberSequence = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  this->RandomNumberSequence->SetSeed(1);
  this->VBOs = 0;
  this->ShaderCache = 0;
  this->CurrentBuffer = 0;
  this->FrameBuffer = 0;
  this->CurrentTexture = 0;
  this->FrameTexture = 0;
  this->Program = 0;
  this->BlendingProgram = 0;
  this->TextureProgram = 0;
  this->IndexBufferObject = 0;
  this->Particles->SetDataTypeToFloat();
  this->RebuildBufferObjects = true;
  this->InterpolationArray = 0;
  this->InterpolationScalarArray = 0;
  this->Vectors = 0;
  this->Scalars = this->Particles->GetData();
  this->DataSet = 0;
  this->ClearFlag = true;
  this->RebuildBufferObjects = true;
  this->Locator = 0;
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
    this->InterpolationArray = 0;
  }
  if (this->InterpolationScalarArray)
  {
    this->InterpolationScalarArray->Delete();
    this->InterpolationScalarArray = 0;
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
  this->Indices.resize(nbParticles * 2);
  if (this->InterpolationScalarArray)
  {
    this->InterpolationScalarArray->Resize(nbParticles * 2);
  }

  // Build indices array
  for (int i = 0; i < nbParticles * 2; i++)
  {
    this->Indices[i] = i;
  }

  this->RebuildBufferObjects = true;
}

//-----------------------------------------------------------------------------
bool vtkStreamLinesMapper::Private::InterpolateSpeedAndColor(
  double pos[3], double outSpeed[3], vtkIdType pid)
{
  int subId;
  double pcoords[3];
  static double weights[1024];

  vtkIdType cellId = 0;
  if (!this->Locator)
  {
    cellId = this->DataSet->FindCell(pos, 0, -1, 1e-10, subId, pcoords, weights);
  }
  else
  {
    cellId = this->Locator->FindCell(pos, 0., this->GenericCell.Get(), pcoords, weights);
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
      this->Vectors->GetTuple(cellId, outSpeed);
    }
    else
    {
      this->InterpolationArray->InterpolateTuple(0, this->IdList.Get(), this->Vectors, weights);
      this->InterpolationArray->GetTuple(0, outSpeed);
    }
    double speed = vtkMath::Norm(outSpeed);
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
      this->InterpolationScalarArray->SetTuple(pid, this->Scalars->GetTuple(cellId));
    }
    else
    {
      this->InterpolationScalarArray->InterpolateTuple(
        pid, this->IdList.Get(), this->Scalars, weights);
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
    double pos[3];
    pos[0] = this->Rand(this->Bounds[0], this->Bounds[1]);
    pos[1] = this->Rand(this->Bounds[2], this->Bounds[3]);
    pos[2] = this->Rand(this->Bounds[4], this->Bounds[5]);
    this->Particles->SetPoint(pid * 2 + 0, pos);
    this->Particles->SetPoint(pid * 2 + 1, pos);
    this->ParticlesTTL[pid] = this->Rand(1, this->Mapper->MaxTimeToLive);

    // Check speed at this location
    double speedVec[9];
    if (this->InterpolateSpeedAndColor(pos, speedVec, pid * 2))
    {
      this->InterpolationScalarArray->SetTuple(
        pid * 2 + 1, this->InterpolationScalarArray->GetTuple(pid * 2));
      double speed = vtkMath::Norm(speedVec);
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
      double pos[3];
      this->Particles->GetPoint(i * 2 + 1, pos);

      // Update prevpos with last pos
      this->Particles->SetPoint(i * 2 + 0, pos);
      this->InterpolationScalarArray->SetTuple(
        i * 2 + 0, this->InterpolationScalarArray->GetTuple(i * 2 + 1));

      // Move the particle and fetch its color
      double speedVec[3];
      if (this->InterpolateSpeedAndColor(pos, speedVec, i * 2 + 1))
      {
        this->Particles->SetPoint(2 * i + 1, pos[0] + dt * speedVec[0], pos[1] + dt * speedVec[1],
          pos[2] + dt * speedVec[2]);
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
  if (!this->PrepareGLBuffers(ren, actor))
  {
    return;
  }

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

  ////////////////////////////////////////////////
  // Pass 1: Render segment to current buffer FBO
  this->CurrentBuffer->SetContext(renWin);
  this->CurrentBuffer->SaveCurrentBindingsAndBuffers();
  this->CurrentBuffer->Bind();
#ifdef VTK_UPDATED_FRAMEBUFFER
  this->CurrentBuffer->AddColorAttachment(0, this->CurrentTexture);
#else
  this->CurrentBuffer->AddColorAttachment(
    this->CurrentBuffer->GetBothMode(), 0, this->CurrentTexture);
#endif
  this->CurrentBuffer->AddDepthAttachment(); // auto create depth buffer
  this->CurrentBuffer->ActivateBuffer(0);
  this->CurrentBuffer->Start(this->CurrentTexture->GetWidth(), this->CurrentTexture->GetHeight());

  this->ShaderCache->ReadyShaderProgram(this->Program);
  if (this->Program->IsUniformUsed("MCDCMatrix"))
  {
    actor->ComputeMatrix();
    if (!actor->GetIsIdentity())
    {
      vtkMatrix4x4* mcwc;
      vtkMatrix3x3* anorms;
      static_cast<vtkOpenGLActor*>(actor)->GetKeyMatrices(mcwc, anorms);
      vtkMatrix4x4::Multiply4x4(mcwc, wcdc, this->TempMatrix4.Get());
      this->Program->SetUniformMatrix("MCDCMatrix", this->TempMatrix4.Get());
    }
    else
    {
      this->Program->SetUniformMatrix("MCDCMatrix", wcdc);
    }
  }

  bool useScalars = this->Scalars && this->Mapper->GetScalarVisibility();
  double* col = actor->GetProperty()->GetDiffuseColor();
  float color[3];
  color[0] = static_cast<double>(col[0]);
  color[1] = static_cast<double>(col[1]);
  color[2] = static_cast<double>(col[2]);
  this->Program->SetUniform3f("color", color);
  this->Program->SetUniformi("scalarVisibility", useScalars);

  if (this->CreateWideLines && this->Program->IsUniformUsed("lineWidthNVC"))
  {
    int vp[4];
    ostate->vtkglGetIntegerv(GL_VIEWPORT, vp);
    float lineWidth[2];
    lineWidth[0] = 2.0 * actor->GetProperty()->GetLineWidth() / vp[2];
    lineWidth[1] = 2.0 * actor->GetProperty()->GetLineWidth() / vp[3];
    this->Program->SetUniform2f("lineWidthNVC", lineWidth);
  }

  // Setup the IBO
  this->IndexBufferObject->Bind();
  if (this->RebuildBufferObjects)
  {
    // We upload the indices only when number of particles changed
    this->IndexBufferObject->Upload(
      &this->Indices[0], nbParticles * 2, vtkOpenGLBufferObject::ElementArrayBuffer);
    this->RebuildBufferObjects = false;
  }

  vtkSmartPointer<vtkUnsignedCharArray> colors = 0;
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
  this->VBOs->AddAllAttributesToVAO(this->Program, vao.Get());

  // Perform rendering
  ostate->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
  ostate->vtkglClear(GL_COLOR_BUFFER_BIT);
  ostate->vtkglDisable(GL_DEPTH_TEST);
  if (!this->CreateWideLines)
  {
    glLineWidth(actor->GetProperty()->GetLineWidth());
  }
  glDrawArrays(GL_LINES, 0, nbParticles * 2);
  vtkOpenGLCheckErrorMacro("Failed after rendering");

  this->IndexBufferObject->Release();
  vao->Release();

  this->CurrentBuffer->UnBind();
  this->CurrentBuffer->RestorePreviousBindingsAndBuffers();

  static float s_quadTCoords[8] = { 0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f };
  static float s_quadVerts[12] = { -1.f, -1.f, 0.f, 1.f, -1.f, 0.f, 1.f, 1.f, 0.f, -1.f, 1.f, 0.f };

  ////////////////////////////////////////////////////////////////////
  // Pass 2: Blend current and previous frame in the frame buffer FBO
  if (animate)
  {
    this->FrameBuffer->SetContext(renWin);
    this->FrameBuffer->SaveCurrentBindingsAndBuffers();
    this->FrameBuffer->Bind();
#ifdef VTK_UPDATED_FRAMEBUFFER
    this->FrameBuffer->AddColorAttachment(0, this->FrameTexture);
#else
    this->FrameBuffer->AddColorAttachment(this->FrameBuffer->GetBothMode(), 0, this->FrameTexture);
#endif
    this->FrameBuffer->AddDepthAttachment(); // auto create depth buffer
    this->FrameBuffer->ActivateBuffer(0);
    this->FrameBuffer->Start(this->FrameTexture->GetWidth(), this->FrameTexture->GetHeight());

    if (this->ClearFlag)
    {
      // Clear frame buffer if camera changed
      ostate->vtkglClear(GL_COLOR_BUFFER_BIT);
      this->CameraMTime = cam->GetMTime();
      this->ClearFlag = false;
    }

    this->ShaderCache->ReadyShaderProgram(this->BlendingProgram);
    vtkNew<vtkOpenGLVertexArrayObject> vaotb;
    vaotb->Bind();
    this->FrameTexture->Activate();
    this->CurrentTexture->Activate();
    double alpha =
      1.0 - (1.0 / (this->Mapper->MaxTimeToLive * std::max(0.00001, this->Mapper->Alpha)));
    this->BlendingProgram->SetUniformf("alpha", alpha);
    this->BlendingProgram->SetUniformi("prev", this->FrameTexture->GetTextureUnit());
    this->BlendingProgram->SetUniformi("current", this->CurrentTexture->GetTextureUnit());
    vtkOpenGLRenderUtilities::RenderQuad(
      s_quadVerts, s_quadTCoords, this->BlendingProgram, vaotb.Get());
    this->CurrentTexture->Deactivate();
    vaotb->Release();

    this->FrameBuffer->UnBind();
    this->FrameBuffer->RestorePreviousBindingsAndBuffers();
  }

  ////////////////////////////////////////////////
  // Pass 3: Finally draw the FBO onto the screen
  if (!this->ClearFlag)
  {
    this->ShaderCache->ReadyShaderProgram(this->TextureProgram);
    vtkNew<vtkOpenGLVertexArrayObject> vaot;
    vaot->Bind();
    this->FrameTexture->Activate();
    this->TextureProgram->SetUniformi("source", this->FrameTexture->GetTextureUnit());
    // Setup blending equation
    int prevBlendParams[4];
    ostate->vtkglGetIntegerv(GL_BLEND_SRC_RGB, &prevBlendParams[0]);
    ostate->vtkglGetIntegerv(GL_BLEND_DST_RGB, &prevBlendParams[1]);
    ostate->vtkglGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendParams[2]);
    ostate->vtkglGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendParams[3]);
    ostate->vtkglEnable(GL_BLEND);
    ostate->vtkglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    vtkOpenGLRenderUtilities::RenderQuad(
      s_quadVerts, s_quadTCoords, this->TextureProgram, vaot.Get());

    // Restore blending equation state
    ostate->vtkglBlendFuncSeparate(
      prevBlendParams[0], prevBlendParams[1], prevBlendParams[2], prevBlendParams[3]);

    this->FrameTexture->Deactivate();
    vaot->Release();
  }
  ostate->vtkglEnable(GL_DEPTH_TEST);
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

  if (!this->ShaderCache)
  {
    this->ShaderCache = renWin->GetShaderCache();
  }

  bool prevCreateWideLines = this->CreateWideLines;
  this->CreateWideLines = actor->GetProperty()->GetLineWidth() > 1.0 &&
    actor->GetProperty()->GetLineWidth() > renWin->GetMaximumHardwareLineWidth();

  if (!this->Program || (prevCreateWideLines != this->CreateWideLines))
  {
    this->ShaderCache->ReleaseCurrentShader();
    if (this->Program)
    {
      RELEASE_VTKGL_OBJECT(this->Program);
    }
    this->Program = this->ShaderCache->ReadyShaderProgram(
      vtkStreamLines_vs, vtkStreamLines_fs, this->CreateWideLines ? vtkStreamLines_gs : "");
    this->Program->Register(this);
  }

  if (!this->BlendingProgram)
  {
    this->BlendingProgram =
      this->ShaderCache->ReadyShaderProgram(vtkTextureObjectVS, vtkStreamLinesBlending_fs, "");
    this->BlendingProgram->Register(this);
  }

  if (!this->TextureProgram)
  {
    this->TextureProgram =
      this->ShaderCache->ReadyShaderProgram(vtkTextureObjectVS, vtkStreamLinesCopy_fs, "");
    this->TextureProgram->Register(this);
  }

  if (!this->IndexBufferObject)
  {
    this->IndexBufferObject = vtkOpenGLBufferObject::New();
    this->IndexBufferObject->SetType(vtkOpenGLBufferObject::ElementArrayBuffer);
  }

  return this->CurrentTexture && this->FrameTexture && this->ShaderCache && this->Program &&
    this->BlendingProgram && this->TextureProgram && this->IndexBufferObject;
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
      this->Locator = 0;
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
      this->InterpolationScalarArray = 0;
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
      this->InterpolationArray = 0;
    }
    this->InterpolationArray = vtkDataArray::CreateDataArray(speedField->GetDataType());
    this->InterpolationArray->SetNumberOfComponents(3);
    this->InterpolationArray->SetNumberOfTuples(1);
  }
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkStreamLinesMapper)

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

  // Set processing dataset and arrays
  this->Internal->SetData(inData, inVectors, inScalars);

  bool animate = true;
  for (int i = 0; i < this->NumberOfAnimationSteps && animate; i++)
  {
    animate = this->Animate &&
      (this->NumberOfAnimationSteps == 1 || (this->NumberOfAnimationSteps > 1 &&
                                              this->AnimationSteps < this->NumberOfAnimationSteps));
    if (animate)
    {
      // Move particles
      this->Internal->UpdateParticles();
      if (this->NumberOfAnimationSteps > 1)
      {
        this->AnimationSteps++;
      }
    }

    // Draw updated particles in a buffer
    this->Internal->DrawParticles(ren, actor, animate);
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
