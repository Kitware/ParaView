/*=========================================================================

   Program: ParaView
   Module:    vtkEDLShading.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
/*----------------------------------------------------------------------
Acknowledgement:
This algorithm is the result of joint work by Electricité de France,
CNRS, Collège de France and Université J. Fourier as part of the
Ph.D. thesis of Christian BOUCHENY.
------------------------------------------------------------------------*/

#include "vtkEDLShading.h"

#include "vtkObjectFactory.h"
#include <assert.h>
#include <string>
#include <sstream>
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkgl.h"
#include "vtkFrameBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTextureUnitManager.h"
#include "vtkPropCollection.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkEDLShading, "$Revision: 1.1 $")
;
vtkStandardNewMacro(vtkEDLShading)
;

extern const char *edl_compose;
extern const char *edl_shade;
extern const char *bilateral_filter;

// ----------------------------------------------------------------------------
vtkEDLShading::vtkEDLShading()
{

  this->ProjectionFBO = 0;
  this->ProjectionColorTexture = 0;
  this->ProjectionDepthTexture = 0;

  this->EDLHighFBO = 0;
  this->EDLHighShadeTexture = 0;
  this->EDLLowFBO = 0;
  this->EDLLowShadeTexture = 0;
  this->EDLLowBlurTexture = 0;

  this->EDLShadeProgram = 0;
  this->EDLComposeProgram = 0;
  this->BilateralProgram = 0;

  EDLIsFiltered = true;
  // init neighbours in image space
  for (int c = 0; c < 8; c++)
    {
    float x, y;
    x = cos(2* 3.14159 * float (c)/8.);
    y = sin(2*3.14159*float(c)/8.);
    EDLNeighbours[4*c] = x / sqrt(x*x+y*y);
    EDLNeighbours[4*c+1] = y / sqrt(x*x+y*y);
    EDLNeighbours[4*c+2] = 0.;
    EDLNeighbours[4*c+3] = 0.;
    }
  EDLLowResFactor = 2;
}

// ----------------------------------------------------------------------------
vtkEDLShading::~vtkEDLShading()
{
  if (this->ProjectionFBO != 0)
    {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in "
      <<"ReleaseGraphicsResources().");
    }
  if (this->ProjectionColorTexture != 0)
    {
    vtkErrorMacro(<<"ColorTexture should have been deleted in "
      <<"ReleaseGraphicsResources().");
    }
  if (this->ProjectionDepthTexture != 0)
    {
    vtkErrorMacro(<<"DepthTexture should have been deleted in "
      <<"ReleaseGraphicsResources().");
    }
  if (this->EDLHighFBO != 0)
    {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in "
      <<"ReleaseGraphicsResources().");
    }
  if (this->EDLHighShadeTexture != 0)
    {
    vtkErrorMacro(<<"ColorTexture should have been deleted in "
      <<"ReleaseGraphicsResources().");
    }
  if (this->EDLLowFBO != 0)
    {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in "
      <<"ReleaseGraphicsResources().");
    }
  if (this->EDLLowShadeTexture != 0)
    {
    vtkErrorMacro(<<"ColorTexture should have been deleted in "
      <<"ReleaseGraphicsResources().");
    }
  if (this->EDLLowBlurTexture != 0)
    {
    vtkErrorMacro(<<"ColorTexture should have been deleted in "
      <<"ReleaseGraphicsResources().");
    }
  if (this->EDLShadeProgram != 0)
    {
    vtkErrorMacro(<<"EDL Shade program should have been deleted in "
      <<"ReleaseGraphicsResources().");
    }
  if (this->EDLComposeProgram != 0)
    {
    vtkErrorMacro(<<"EDL Compose program should have been deleted in "
      <<"ReleaseGraphicsResources().");
    }
  if (this->BilateralProgram != 0)
   {
   vtkErrorMacro(<<"Bilateral Filter program should have been deleted in "
     <<"ReleaseGraphicsResources().");
   }
}

// ----------------------------------------------------------------------------
void vtkEDLShading::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DelegatePass:";
  if (this->DelegatePass != 0)
    {
    this->DelegatePass->PrintSelf(os, indent);
    }
  else
    {
    os << "(none)" << endl;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Initialize framebuffers and associated texture objects,
// with link to render state s
void vtkEDLShading::EDLInitializeFramebuffers(vtkRenderState &s)
{
  vtkRenderer *r = s.GetRenderer();

  //  PROJECTION FBO and TEXTURES
  //
  if (this->ProjectionFBO == 0)
    {
    this->ProjectionFBO = vtkFrameBufferObject::New();
    this->ProjectionFBO->SetContext(r->GetRenderWindow());
    }
  s.SetFrameBuffer(this->ProjectionFBO);
  this->ProjectionFBO->Bind();
  // Color texture
  if (this->ProjectionColorTexture == 0)
    {
    this->ProjectionColorTexture = vtkTextureObject::New();
    this->ProjectionColorTexture->SetContext(this->ProjectionFBO->GetContext());
    }
  if (this->ProjectionColorTexture->GetWidth() != static_cast<unsigned int> (w)
      || this->ProjectionColorTexture->GetHeight()
          != static_cast<unsigned int> (h))
    {
    this->ProjectionColorTexture->Bind();
    this->ProjectionColorTexture->Create2D(w, h, 4, VTK_FLOAT, false);
    }
  // Depth texture
  if (this->ProjectionDepthTexture == 0)
    {
    this->ProjectionDepthTexture = vtkTextureObject::New();
    this->ProjectionDepthTexture->SetContext(this->ProjectionFBO->GetContext());
    }
  if (this->ProjectionDepthTexture->GetWidth() != static_cast<unsigned int> (w)
      || this->ProjectionDepthTexture->GetHeight()
          != static_cast<unsigned int> (h))
    {
    this->ProjectionDepthTexture->Bind();
    this->ProjectionDepthTexture->Create2D(w, h, 1, VTK_VOID, false);
    }
  // Apply textures
  // to make things clear, we write all
  this->ProjectionFBO->SetNumberOfRenderTargets(1);
  this->ProjectionFBO->SetColorBuffer(0, this->ProjectionColorTexture);
  this->ProjectionFBO->SetActiveBuffer(0);
  this->ProjectionFBO->SetDepthBuffer(this->ProjectionDepthTexture);

  this->ProjectionDepthTexture->SetWrapS(vtkTextureObject::ClampToEdge);
  this->ProjectionDepthTexture->SetWrapT(vtkTextureObject::ClampToEdge);
  this->ProjectionDepthTexture->SetMinificationFilter(vtkTextureObject::Linear);
  this->ProjectionDepthTexture->SetLinearMagnification(true);
  this->ProjectionDepthTexture->Bind();
  this->ProjectionDepthTexture->SendParameters();

  this->ProjectionFBO->UnBind();

  //  EDL-RES1 FBO and TEXTURE
  //
  if (this->EDLHighFBO == 0)
    {
    this->EDLHighFBO = vtkFrameBufferObject::New();
    this->EDLHighFBO->SetContext(r->GetRenderWindow());
    }
  s.SetFrameBuffer(EDLHighFBO);
  // Color texture
  if (this->EDLHighShadeTexture == 0)
    {
    this->EDLHighShadeTexture = vtkTextureObject::New();
    this->EDLHighShadeTexture->SetContext(this->EDLHighFBO->GetContext());
    }
  if (this->EDLHighShadeTexture->GetWidth() != static_cast<unsigned int> (w)
      || this->EDLHighShadeTexture->GetHeight()
          != static_cast<unsigned int> (h))
    {
    this->EDLHighShadeTexture->Create2D(w, h, 4, VTK_FLOAT, false);
    }
  this->EDLHighFBO->SetNumberOfRenderTargets(1);
  this->EDLHighFBO->SetColorBuffer(0, this->EDLHighShadeTexture);
  this->EDLHighFBO->SetActiveBuffer(0);
  this->EDLHighFBO->SetDepthBufferNeeded(false);
  this->EDLHighFBO->UnBind();

  //  EDL-RES2 FBO and TEXTURE
  //
  if (this->EDLLowFBO == 0)
    {
    this->EDLLowFBO = vtkFrameBufferObject::New();
    this->EDLLowFBO->SetContext(r->GetRenderWindow());
    }
  s.SetFrameBuffer(EDLLowFBO);
  // Color texture
  if (this->EDLLowShadeTexture == 0)
    {
    this->EDLLowShadeTexture = vtkTextureObject::New();
    this->EDLLowShadeTexture->SetContext(this->EDLLowFBO->GetContext());
    }
  if (this->EDLLowShadeTexture->GetWidth() != static_cast<unsigned int> (w
      / EDLLowResFactor) || this->EDLLowShadeTexture->GetHeight()
      != static_cast<unsigned int> (h / EDLLowResFactor))
    {
    this->EDLLowShadeTexture->Create2D(w / EDLLowResFactor,
        h / EDLLowResFactor, 4, VTK_FLOAT, false);
    }
  // Blur texture
  if (this->EDLLowBlurTexture == 0)
    {
    this->EDLLowBlurTexture = vtkTextureObject::New();
    this->EDLLowBlurTexture->SetContext(this->EDLLowFBO->GetContext());
    }
  if (this->EDLLowBlurTexture->GetWidth() != static_cast<unsigned int> (w
      / EDLLowResFactor) || this->EDLLowBlurTexture->GetHeight()
      != static_cast<unsigned int> (h / EDLLowResFactor))
    {
    this->EDLLowBlurTexture->Create2D(w / EDLLowResFactor, h / EDLLowResFactor,
        4, VTK_FLOAT, false);
    }
  this->EDLLowFBO->SetNumberOfRenderTargets(1);
  this->EDLLowFBO->SetColorBuffer(0, this->EDLLowShadeTexture);
  this->EDLLowFBO->SetActiveBuffer(0);
  this->EDLLowFBO->SetDepthBufferNeeded(false);

  this->EDLLowShadeTexture->SetWrapS(vtkTextureObject::ClampToEdge);
  this->EDLLowShadeTexture->SetWrapT(vtkTextureObject::ClampToEdge);
  this->EDLLowShadeTexture->SetMinificationFilter(vtkTextureObject::Linear);
  this->EDLLowShadeTexture->SetLinearMagnification(true);
  this->EDLLowShadeTexture->Bind();
  this->EDLLowShadeTexture->SendParameters();

  this->EDLLowBlurTexture->SetWrapS(vtkTextureObject::ClampToEdge);
  this->EDLLowBlurTexture->SetWrapT(vtkTextureObject::ClampToEdge);
  this->EDLLowBlurTexture->SetMinificationFilter(vtkTextureObject::Linear);
  this->EDLLowBlurTexture->SetLinearMagnification(true);
  this->EDLLowBlurTexture->Bind();
  this->EDLLowBlurTexture->SendParameters();

  this->EDLLowFBO->UnBind();
}
// ----------------------------------------------------------------------------
// Description:
// Initialize shaders
//
void vtkEDLShading::EDLInitializeShaders()
{
#ifdef VTK_EDL_SHADING_DEBUG
  cout << "EDL: INITIALIZE SHADERS" << endl;
#endif
  //  EDL SHADE
  //
  if (this->EDLShadeProgram == 0)
    {
    this->EDLShadeProgram = vtkShaderProgram2::New();
    this->EDLShadeProgram->SetContext(
            static_cast<vtkOpenGLRenderWindow *>
            (this->ProjectionFBO->GetContext()));
    vtkShader2 *shader = vtkShader2::New();
    shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
    shader->SetSourceCode(edl_shade);
    shader->SetContext(this->EDLShadeProgram->GetContext());
    this->EDLShadeProgram->GetShaders()->AddItem(shader);
    shader->Delete();
    this->EDLShadeProgram->Build();
    }
#ifdef VTK_EDL_SHADING_DEBUG
  this->EDLShadeProgram->PrintActiveUniformVariablesOnCout();
#endif

  //  EDL COMPOSE
  //
  if (this->EDLComposeProgram == 0)
    {
    this->EDLComposeProgram = vtkShaderProgram2::New();
    this->EDLComposeProgram->SetContext(
        static_cast<vtkOpenGLRenderWindow *> (this->EDLHighFBO->GetContext()));
    vtkShader2 *shader = vtkShader2::New();
    shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
    shader->SetSourceCode(edl_compose);
    shader->SetContext(this->EDLComposeProgram->GetContext());
    this->EDLComposeProgram->GetShaders()->AddItem(shader);
    shader->Delete();
    this->EDLComposeProgram->Build();
    }
#ifdef VTK_EDL_SHADING_DEBUG
  this->EDLComposeProgram->PrintActiveUniformVariablesOnCout();
#endif

  //  BILATERAL FILTER
  //
  if (this->BilateralProgram == 0)
    {
    this->BilateralProgram = vtkShaderProgram2::New();
    this->BilateralProgram->SetContext(
        static_cast<vtkOpenGLRenderWindow *> (this->EDLLowFBO->GetContext()));
    vtkShader2 *shader = vtkShader2::New();
    shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
    shader->SetSourceCode(bilateral_filter);
    shader->SetContext(this->BilateralProgram->GetContext());
    this->BilateralProgram->GetShaders()->AddItem(shader);
    shader->Delete();
    this->BilateralProgram->Build();
    }
#ifdef VTK_EDL_SHADING_DEBUG
  this->BilateralProgram->PrintActiveUniformVariablesOnCout();
#endif

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "... done" << endl;
#endif
}

// ----------------------------------------------------------------------------
// Description:
// Render EDL in full resolution
//
bool vtkEDLShading::EDLShadeHigh(vtkRenderState &s)
{
  //  VARIABLES
  //
  vtkRenderer *r = s.GetRenderer();
  vtkUniformVariables *var;
  vtkTextureUnitManager *tu;
  int sourceIdZ;
  float d = 1.0;
  float F_scale = 5.0;
  float SX = 1. / float(w);
  float SY = 1. / float(h);
  float L[3] =
    { 0., 0., -1. };

  // ACTIVATE FBO
  //
  s.SetFrameBuffer(this->EDLHighFBO);
  this->EDLHighFBO->Start(w, h, false);
  this->EDLHighFBO->SetColorBuffer(0, this->EDLHighShadeTexture);
  this->EDLHighFBO->SetActiveBuffer(0);

  // ACTIVATE SHADER
  //
  if (this->EDLShadeProgram->GetLastBuildStatus()
      != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro(<<"Couldn't build the shader program. At this point ,"
                  <<" it can be an error in a shader or a driver bug.");
    // restore some state.
    this->EDLHighFBO->UnBind();
    return false;
    }
  //
  var = this->EDLShadeProgram->GetUniformVariables();
  tu = static_cast<vtkOpenGLRenderWindow *>
         (r->GetRenderWindow())->GetTextureUnitManager();
  // DEPTH TEXTURE PARAMETERS
  sourceIdZ = tu->Allocate();
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdZ);
  this->ProjectionDepthTexture->Bind();

  // shader parameters
  var->SetUniformi("s2_depth", 1, &sourceIdZ);
  var->SetUniformf("d", 1, &d);
  var->SetUniformf("F_scale", 1, &F_scale);
  var->SetUniformf("SX", 1, &SX);
  var->SetUniformf("SY", 1, &SY);
  var->SetUniformf("L", 3, L);
  var->SetUniformfv("N", 4, 8, EDLNeighbours);
  var->SetUniformf("Znear", 1, &Zn);
  var->SetUniformf("Zfar", 1, &Zf);

  // compute the scene bounding box, and set the scene size to the diagonal of it.
  double bb[6];
  vtkMath::UninitializeBounds(bb);
  for(int i=0; i<s.GetPropArrayCount(); i++)
    {
    double* bounds = s.GetPropArray()[i]->GetBounds();
    if(i==0)
      {
      bb[0] = bounds[0];
      bb[1] = bounds[1];
      bb[2] = bounds[2];
      bb[3] = bounds[3];
      bb[4] = bounds[4];
      bb[5] = bounds[5];
      }
    else
      {
      bb[0] = (bb[0] < bounds[0] ? bb[0] : bounds[0]);
      bb[1] = (bb[1] > bounds[1] ? bb[1] : bounds[1]);
      bb[2] = (bb[2] < bounds[2] ? bb[2] : bounds[2]);
      bb[3] = (bb[3] > bounds[3] ? bb[3] : bounds[3]);
      bb[4] = (bb[4] < bounds[4] ? bb[4] : bounds[4]);
      bb[5] = (bb[5] > bounds[5] ? bb[5] : bounds[5]);
      }
    }

  float diag = (bb[1]-bb[0])*(bb[1]-bb[0]) + (bb[3]-bb[2])*(bb[3]-bb[2])
               + (bb[5]-bb[4])*(bb[5]-bb[4]);
  diag = sqrt(diag);
   var->SetUniformf("SceneSize", 1, &diag);
  //
  this->EDLShadeProgram->Use();
  if (!this->EDLShadeProgram->IsValid())
    {
    vtkErrorMacro(<<this->EDLShadeProgram->GetLastValidateLog());

    return false;
    }

  // RENDER AND FREE ALL
  EDLHighFBO->RenderQuad(0, w - 1, 0, h - 1);
  //
  this->EDLShadeProgram->Restore();
  tu->Free(sourceIdZ);
  this->ProjectionDepthTexture->UnBind();
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  this->EDLHighFBO->UnBind();

  return true; // succeeded
}

// ----------------------------------------------------------------------------
// Description:
// Render EDL in low resolution
//
bool vtkEDLShading::EDLShadeLow(vtkRenderState &s)
{
  //  VARIABLES
  //
  vtkRenderer *r = s.GetRenderer();
  vtkUniformVariables *var;
  vtkTextureUnitManager *tu;
  int sourceIdZ;
  float d = 2.0;
  float F_scale = 5.0;
  float SX = 1. / float(w / EDLLowResFactor);
  float SY = 1. / float(h / EDLLowResFactor);
  float L[3] =
    { 0., 0., -1. };

  // ACTIVATE FBO
  //
  s.SetFrameBuffer(this->EDLLowFBO);
  this->EDLLowFBO->Start(w / EDLLowResFactor, h / EDLLowResFactor, false);
  this->EDLLowFBO->SetColorBuffer(0, this->EDLLowBlurTexture);
  this->EDLLowBlurTexture->SetLinearMagnification(true);
  this->EDLLowBlurTexture->Bind();
  this->EDLLowBlurTexture->SendParameters();
  this->EDLLowFBO->SetActiveBuffer(0);

  // ACTIVATE SHADER
  //
  if (this->EDLShadeProgram->GetLastBuildStatus()
      != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro(<<"Couldn't build the shader program. At this point ,"
                  <<" it can be an error in a shader or a driver bug.");
    // restore some state.
    return false;
    }
  //
  var = this->EDLShadeProgram->GetUniformVariables();
  tu
      = static_cast<vtkOpenGLRenderWindow *> (r->GetRenderWindow())->GetTextureUnitManager();
  // depth texture parameters
  sourceIdZ = tu->Allocate();
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdZ);
  this->ProjectionDepthTexture->Bind();
  // shader parameters
  var->SetUniformi("s2_depth", 1, &sourceIdZ);
  var->SetUniformf("d", 1, &d);
  var->SetUniformf("F_scale", 1, &F_scale);
  var->SetUniformf("SX", 1, &SX);
  var->SetUniformf("SY", 1, &SY);
  var->SetUniformf("L", 3, L);
  var->SetUniformfv("N", 4, 8, EDLNeighbours); // USELESS, ALREADY DEFINED IN FULL RES
  var->SetUniformf("Znear", 1, &Zn);
  var->SetUniformf("Zfar", 1, &Zf);
  //
  this->EDLShadeProgram->Use();
  if (!this->EDLShadeProgram->IsValid())
    {
    vtkErrorMacro(<<this->EDLShadeProgram->GetLastValidateLog());
    return false;
    }

  // RENDER AND FREE ALL
  //
  EDLLowFBO->RenderQuad(0, w / EDLLowResFactor - 1, 0, h / EDLLowResFactor - 1);
  //
  this->EDLShadeProgram->Restore();
  tu->Free(sourceIdZ);
  this->ProjectionDepthTexture->UnBind();
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  this->EDLLowFBO->UnBind();

  return true; // succeeded
}

// ----------------------------------------------------------------------------
// Description:
// Bilateral Filter low resolution shaded image
//
bool vtkEDLShading::EDLBlurLow(vtkRenderState &s)
{
  //  VARIABLES
  //
  vtkRenderer *r = s.GetRenderer();
  vtkUniformVariables *var;
  vtkTextureUnitManager *tu;
  int sourceIdZ;
  int sourceIdEDL;
  // shader parameters
  float SX = 1. / float(this->w / EDLLowResFactor);
  float SY = 1. / float(this->h / EDLLowResFactor);
  int EDL_Bilateral_N = 5;
  float EDL_Bilateral_Sigma = 2.5;

  // ACTIVATE FBO
  //
  s.SetFrameBuffer(this->EDLLowFBO);
  this->EDLLowFBO->Start(this->w / EDLLowResFactor, this->h / EDLLowResFactor,
      false);
  this->EDLLowFBO->SetColorBuffer(0, this->EDLLowBlurTexture);
  this->EDLLowFBO->SetActiveBuffer(0);

  // ACTIVATE SHADER
  //
  if (this->BilateralProgram->GetLastBuildStatus()
      != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro(<<"Couldn't build the shader program. At this point ,"
                  <<" it can be an error in a shader or a driver bug.");
    this->EDLLowFBO->UnBind();
    EDLIsFiltered = false;
    return EDLIsFiltered;
    }

  //
  var = this->BilateralProgram->GetUniformVariables();
  tu = static_cast<vtkOpenGLRenderWindow *>
         (r->GetRenderWindow())->GetTextureUnitManager();
  // DEPTH TEXTURE PARAMETERS
  sourceIdZ = tu->Allocate();
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdZ);
  this->ProjectionDepthTexture->Bind();
  // SHADING MAP
  sourceIdEDL = tu->Allocate();
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdEDL);
  this->EDLLowShadeTexture->Bind();

  // shader parameters
  var->SetUniformi("s2_I", 1, &sourceIdEDL);
  var->SetUniformi("s2_D", 1, &sourceIdZ);
  var->SetUniformf("SX", 1, &SX);
  var->SetUniformf("SY", 1, &SY);
  var->SetUniformi("N", 1, &EDL_Bilateral_N);
  var->SetUniformf("sigma", 1, &EDL_Bilateral_Sigma);

  this->BilateralProgram->Use();
  if (!this->BilateralProgram->IsValid())
    {
    vtkErrorMacro(<<this->BilateralProgram->GetLastValidateLog());
    EDLIsFiltered = false;
    }
  else
    {
    // RENDER SSAO AND FREE ALL
    //
    EDLLowFBO->RenderQuad(0, w / EDLLowResFactor - 1, 0,
                          h / EDLLowResFactor - 1);
    }
  this->BilateralProgram->Restore();
  tu->Free(sourceIdEDL);
  this->EDLLowShadeTexture->UnBind();
  tu->Free(sourceIdZ);
  this->ProjectionDepthTexture->UnBind();
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);

  this->EDLLowFBO->UnBind();

  return EDLIsFiltered;
}

// ----------------------------------------------------------------------------
// Description:
// Compose color and shaded images
//
bool vtkEDLShading::EDLCompose(const vtkRenderState *s)
{
  //  this->EDLIsFiltered = true;

  vtkRenderer *r = s->GetRenderer();

  //  VARIABLES
  //
  vtkUniformVariables *var;
  vtkTextureUnitManager *tu;
  int sourceIdS1;
  int sourceIdS2;
  int sourceIdZ;
  int sourceIdC;

  // ACTIVATE SHADER
  //
  if (this->EDLComposeProgram->GetLastBuildStatus()
      != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro(<<"Couldn't build the shader program. At this point ,"
                  <<" it can be an error in a shader or a driver bug.");
    // restore some state.
    return false;
    }
  var = this->EDLComposeProgram->GetUniformVariables();
  tu = static_cast<vtkOpenGLRenderWindow *> (r->GetRenderWindow())->GetTextureUnitManager();
  sourceIdS1 = tu->Allocate();
  sourceIdS2 = tu->Allocate();
  sourceIdC = tu->Allocate();
  sourceIdZ = tu->Allocate();
  //  EDL shaded texture - full res
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdS1);
  this->EDLHighShadeTexture->Bind();
  var->SetUniformi("s2_S1", 1, &sourceIdS1);
  //  EDL shaded texture - low res
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdS2);
  //this->EDLLowBlurTexture->SetLinearMagnification(true);
  //this->EDLLowBlurTexture->SendParameters();
  if (EDLIsFiltered)
    this->EDLLowBlurTexture->Bind();
  else
    this->EDLLowShadeTexture->Bind();
  var->SetUniformi("s2_S2", 1, &sourceIdS2);
  //  initial color texture
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdC);
  this->ProjectionColorTexture->Bind();
  var->SetUniformi("s2_C", 1, &sourceIdC);
  //  initial depth texture
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdZ);
  this->ProjectionDepthTexture->Bind();
  var->SetUniformi("s2_Z", 1, &sourceIdZ);
  //
  //var->SetUniformf("Zn",1,&Zn);
  //var->SetUniformf("Zf",1,&Zf);
  this->EDLComposeProgram->Use();

  //  DRAW CONTEXT - prepare blitting
  //
  // Prepare blitting
  glClearColor(1., 1., 1., 1.);
  glClearDepth(1.);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     // IMPORTANT since we enable depth writing hereafter
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
     // IMPORTANT : so that depth information is propagated
  glDisable(GL_LIGHTING);
  glDisable(GL_SCISSOR_TEST);

  this->EDLHighShadeTexture->CopyToFrameBuffer( 0,  0,
      this->w - 1 - 2 * this->extraPixels,
      this->h - 1 - 2 * this->extraPixels, 0, 0,
      this->width, this->height );

  //  FREE ALL
  //
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdS2);
  this->EDLLowShadeTexture->UnBind();
  tu->Free(sourceIdS2);
  //
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdS1);
  this->EDLHighShadeTexture->UnBind();
  tu->Free(sourceIdS1);
  //
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdC);
  this->ProjectionColorTexture->UnBind();
  tu->Free(sourceIdC);
  //
  vtkgl::ActiveTexture(vtkgl::TEXTURE0 + sourceIdZ);
  this->ProjectionDepthTexture->UnBind();
  tu->Free(sourceIdZ);
  //
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  this->EDLComposeProgram->Restore();

  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkEDLShading::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "EDL: Start rendering" << endl;
#endif

  this->NumberOfRenderedProps = 0;
  vtkRenderer *r = s->GetRenderer();

  if (this->DelegatePass != 0)
    {
    //////////////////////////////////////////////////////
    //
    // 1. TEST FOR HARDWARE SUPPORT.
    //    If not supported, just render the delegate.
    //
    if (!this->TestHardwareSupport(s))
      {
      this->DelegatePass->Render(s);
      this->NumberOfRenderedProps
          += this->DelegatePass->GetNumberOfRenderedProps();
      return;
      }
    GLint savedDrawBuffer;
    glGetIntegerv(GL_DRAW_BUFFER, &savedDrawBuffer);

    //////////////////////////////////////////////////////
    //
    //  2. DEFINE SIZE and ACCORDING RENDER STATE
    //
    this->ReadWindowSize(s);
    // this->extraPixels = 20; Obsolete
    this->extraPixels = 0; // extra pixels to zero in the new system
    this->w = this->width + 2* this ->extraPixels;
    this->h=this->height+2*this->extraPixels;
    vtkRenderState s2(r);
    s2.SetPropArrayAndCount(s->GetPropArray(),s->GetPropArrayCount());

    //////////////////////////////////////////////////////
        //
        // 3. INITIALIZE FBOs and SHADERS
        //
        //  FBOs
        //
#ifdef VTK_EDL_SHADING_DEBUG
        cout << "EDL: initializing shaders framebuffers" << endl;
#endif
        this->EDLInitializeFramebuffers(s2);
#ifdef VTK_EDL_SHADING_DEBUG
        cout << "... OK" << endl;
        glFinish();
#endif
        //  Shaders
        //
#ifdef VTK_EDL_SHADING_DEBUG
        cout << "EDL: initializing shaders" << endl;
#endif
        EDLInitializeShaders();
#ifdef VTK_EDL_SHADING_DEBUG
        cout << "... OK" << endl;
        glFinish();
#endif

        //////////////////////////////////////////////////////
        //
        // 4. DELEGATE RENDER IN PROJECTION FBO
        //
#ifdef VTK_EDL_SHADING_DEBUG
        cout << "EDL: Stard 3D rendering" << endl;
#endif
        //
        double znear,zfar;
        r->GetActiveCamera()->GetClippingRange(znear,zfar);
        this->Zf = zfar;
        this->Zn = znear;
        //cout << " -- ZNEAR/ZFAR : " << Zn << " || " << Zf << endl;
        this->ProjectionFBO->Bind();
        this->RenderDelegate(s,this->width,this->height,
             this->w,this->h,this->ProjectionFBO,
             this->ProjectionColorTexture,this->ProjectionDepthTexture);

  this->ProjectionFBO->UnBind();

  glPushAttrib(GL_ALL_ATTRIB_BITS);
#ifdef VTK_EDL_SHADING_DEBUG
  cout << "... OK" << endl;
  glFinish();
#endif

  //system("PAUSE");

  //////////////////////////////////////////////////////
  //
  // 5. EDL SHADING PASS - FULL RESOLUTION
  //
#if EDL_HIGH_RESOLUTION_ON

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "EDL: Shading at full res" << endl;
#endif
  if(! EDLShadeHigh(s2) )
    glDrawBuffer(savedDrawBuffer);

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "... done" << endl;
  glFinish();
#endif

#endif // EDL_HIGH_RESOLUTION_ON

  //////////////////////////////////////////////////////
  //
  // 6. EDL SHADING PASS - LOW RESOLUTION + blur pass
  //
#if EDL_LOW_RESOLUTION_ON

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "EDL: Shading at low res" << endl;
#endif
  if(! EDLShadeLow(s2) )
    glDrawBuffer(savedDrawBuffer);

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "... done" << endl;
  glFinish();
#endif

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "EDL: Bilateral Filtering low res" << endl;
#endif
  if(EDLIsFiltered)
    EDLBlurLow(s2);

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "... done" << endl;
  glFinish();
#endif

#endif // EDL_LOW_RESOLUTION_ON


  //////////////////////////////////////////////////////
  //
  // 7. COMPOSITING PASS (in original framebuffer)
  //
#ifdef VTK_EDL_SHADING_DEBUG
  cout << "EDL: Compose to original fbo" << endl;
#endif

  if(s->GetFrameBuffer() != NULL)
    s->GetFrameBuffer()->Bind();

  glDrawBuffer(savedDrawBuffer);

  if( ! this->EDLCompose(s))
  {
    glDrawBuffer(savedDrawBuffer);
    return;
  }

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "... OK" << endl;
  glFinish();
#endif

#ifdef VTK_EDL_SHADING_DEBUG
  cout << "Exit EDL" << endl;
  glFinish();
#endif
    glPopAttrib();
  }
    else
      {
      vtkWarningMacro(<<" no delegate.");
      }
    }

// --------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkEDLShading::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);

  //  SHADERS
  if (this->EDLShadeProgram != 0)
    {
    this->EDLShadeProgram->ReleaseGraphicsResources();
    this->EDLShadeProgram = 0;
    }
  if (this->EDLComposeProgram != 0)
    {
    this->EDLComposeProgram->ReleaseGraphicsResources();
    this->EDLComposeProgram = 0;
    }
  if (this->BilateralProgram != 0)
    {
    this->BilateralProgram->ReleaseGraphicsResources();
    this->BilateralProgram = 0;
    }

  // FBOs and TOs
  //
  if (this->ProjectionFBO != 0)
    {
    this->ProjectionFBO->Delete();
    this->ProjectionFBO = 0;
    }
  if (this->ProjectionColorTexture != 0)
    {
    this->ProjectionColorTexture->Delete();
    this->ProjectionColorTexture = 0;
    }
  if (this->ProjectionDepthTexture != 0)
    {
    this->ProjectionDepthTexture->Delete();
    this->ProjectionDepthTexture = 0;
    }
  if (this->EDLHighFBO != 0)
    {
    this->EDLHighFBO->Delete();
    this->EDLHighFBO = 0;
    }
  if (this->EDLHighShadeTexture != 0)
    {
    this->EDLHighShadeTexture->Delete();
    this->EDLHighShadeTexture = 0;
    }
  if (this->EDLLowFBO != 0)
    {
    this->EDLLowFBO->Delete();
    this->EDLLowFBO = 0;
    }
  if (this->EDLLowShadeTexture != 0)
    {
    this->EDLLowShadeTexture->Delete();
    this->EDLLowShadeTexture = 0;
    }
  if (this->EDLLowBlurTexture != 0)
    {
    this->EDLLowBlurTexture->Delete();
    this->EDLLowBlurTexture = 0;
    }
}
