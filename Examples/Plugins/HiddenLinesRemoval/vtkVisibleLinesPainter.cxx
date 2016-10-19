/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisibleLinesPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVisibleLinesPainter.h"

#include "vtkActor.h"
#include "vtkBoundingBox.h"
#include "vtkFrameBufferObject.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPVColorMaterialHelper.h"
#include "vtkPVLightingHelper.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkShaderProgram2.h"
#include "vtkTextureObject.h"
#include "vtkUniformVariables.h"

#include "vtkgl.h"
#include <assert.h>
#include <string>

extern const char* vtkVisibleLinesPainter_vs;
extern const char* vtkVisibleLinesPainter_fs;

vtkStandardNewMacro(vtkVisibleLinesPainter);
#define vtkGetIndex(r, c) (c * 4 + r)

inline double vtkClamp(double val, const double& min, const double& max)
{
  val = (val < min) ? min : val;
  val = (val > max) ? max : val;
  return val;
}

class vtkVisibleLinesPainter::vtkInternals
{
public:
  vtkWeakPointer<vtkRenderWindow> LastContext;
  int LastViewportSize[2];
  int ViewportExtent[4];

  vtkSmartPointer<vtkFrameBufferObject> FBO;
  vtkSmartPointer<vtkTextureObject> DepthImage;
  vtkSmartPointer<vtkShaderProgram2> Shader;
  vtkSmartPointer<vtkPVLightingHelper> LightingHelper;
  vtkSmartPointer<vtkPVColorMaterialHelper> ColorMaterialHelper;

  vtkInternals()
  {
    this->LastViewportSize[0] = this->LastViewportSize[1] = 0;
    this->LightingHelper = vtkSmartPointer<vtkPVLightingHelper>::New();
    this->ColorMaterialHelper = vtkSmartPointer<vtkPVColorMaterialHelper>::New();
  }

  void ClearTextures()
  {
    this->DepthImage = 0;
    if (this->FBO)
    {
      this->FBO->RemoveAllColorBuffers();
      this->FBO->RemoveDepthBuffer();
    }
  }

  void ClearGraphicsResources()
  {
    this->ClearTextures();
    this->FBO = 0;
    this->DepthImage = 0;
    this->LightingHelper->Initialize(0, VTK_SHADER_TYPE_VERTEX);
    this->ColorMaterialHelper->Initialize(0);
    if (this->Shader != 0)
    {
      this->Shader->ReleaseGraphicsResources();
      this->Shader = 0;
    }
  }
};

//----------------------------------------------------------------------------
vtkVisibleLinesPainter::vtkVisibleLinesPainter()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkVisibleLinesPainter::~vtkVisibleLinesPainter()
{
  this->ReleaseGraphicsResources(this->Internals->LastContext);
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkVisibleLinesPainter::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Internals->ClearGraphicsResources();
  this->Internals->LastContext = 0;

  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
bool vtkVisibleLinesPainter::CanRender(vtkRenderer* vtkNotUsed(renderer), vtkActor* actor)
{
  return (actor->GetProperty()->GetRepresentation() == VTK_WIREFRAME);
}

//----------------------------------------------------------------------------
void vtkVisibleLinesPainter::PrepareForRendering(vtkRenderer* renderer, vtkActor* actor)
{
  if (!this->CanRender(renderer, actor))
  {
    this->Internals->ClearGraphicsResources();
    this->Internals->LastContext = 0;
    this->Superclass::PrepareForRendering(renderer, actor);
    return;
  }

  vtkRenderWindow* renWin = renderer->GetRenderWindow();
  if (this->Internals->LastContext != renWin)
  {
    this->Internals->ClearGraphicsResources();
  }
  this->Internals->LastContext = renWin;

  int viewsize[2], vieworigin[2];
  renderer->GetTiledSizeAndOrigin(&viewsize[0], &viewsize[1], &vieworigin[0], &vieworigin[1]);
  if (this->Internals->LastViewportSize[0] != viewsize[0] ||
    this->Internals->LastViewportSize[1] != viewsize[1])
  {
    // View size has changed, we need to re-generate the textures.
    this->Internals->ClearTextures();
  }
  this->Internals->LastViewportSize[0] = viewsize[0];
  this->Internals->LastViewportSize[1] = viewsize[1];

  if (!this->Internals->FBO)
  {
    vtkFrameBufferObject* fbo = vtkFrameBufferObject::New();
    fbo->SetContext(renWin);
    this->Internals->FBO = fbo;
    fbo->Delete();
  }

  if (!this->Internals->DepthImage)
  {
    vtkTextureObject* depthImage = vtkTextureObject::New();
    depthImage->SetContext(renWin);
    depthImage->Create2D(viewsize[0], viewsize[1], 1, VTK_VOID, false);
    this->Internals->FBO->SetDepthBuffer(depthImage);
    this->Internals->DepthImage = depthImage;
    depthImage->Delete();
  }

  if (!this->Internals->Shader)
  {
    vtkShaderProgram2* pgm = vtkShaderProgram2::New();
    pgm->SetContext(static_cast<vtkOpenGLRenderWindow*>(renWin));

    vtkShader2* s1 = vtkShader2::New();
    s1->SetType(VTK_SHADER_TYPE_VERTEX);
    s1->SetSourceCode(vtkVisibleLinesPainter_vs);
    s1->SetContext(pgm->GetContext());

    vtkShader2* s2 = vtkShader2::New();
    s2->SetType(VTK_SHADER_TYPE_FRAGMENT);
    s2->SetSourceCode(vtkVisibleLinesPainter_fs);
    s2->SetContext(pgm->GetContext());

    pgm->GetShaders()->AddItem(s1);
    pgm->GetShaders()->AddItem(s2);
    s1->Delete();
    s2->Delete();

    this->Internals->LightingHelper->Initialize(pgm, VTK_SHADER_TYPE_VERTEX);
    this->Internals->ColorMaterialHelper->Initialize(pgm);
    this->Internals->Shader = pgm;
    pgm->Delete();
  }

  // Now compute the bounds of the pixels that this dataset is going to occupy
  // on the screen.
  this->Internals->ViewportExtent[0] = vieworigin[0];
  this->Internals->ViewportExtent[1] = vieworigin[0] + viewsize[0];
  this->Internals->ViewportExtent[2] = vieworigin[1];
  this->Internals->ViewportExtent[3] = vieworigin[1] + viewsize[1];
  this->Superclass::PrepareForRendering(renderer, actor);
}

//----------------------------------------------------------------------------
void vtkVisibleLinesPainter::RenderInternal(
  vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags, bool forceCompileOnly)
{
  if (!this->CanRender(renderer, actor))
  {
    this->Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);
    return;
  }

  vtkRenderWindow* renWin = renderer->GetRenderWindow();

  // Save context state to be able to restore.
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  glDisable(GL_POLYGON_OFFSET_FILL);
  glDisable(GL_POLYGON_OFFSET_LINE);
  glDisable(GL_POLYGON_OFFSET_POINT);

  // we get the view port size (not the renderwindow size).
  int viewsize[2], vieworigin[2];
  renderer->GetTiledSizeAndOrigin(&viewsize[0], &viewsize[1], &vieworigin[0], &vieworigin[1]);

  // Pass One: Render surface, we are only interested in the depth buffer, hence
  // we don't clear the color buffer. However, color buffer attachment is needed
  // for FBO completeness (verify).
  this->Internals->FBO->StartNonOrtho(viewsize[0], viewsize[1], false);
  glClear(GL_DEPTH_BUFFER_BIT);
  this->Superclass::Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);
  glFlush();
  this->Internals->FBO->UnBind();

  // Now paste back the rendered image into the default framebuffer.
  renWin->MakeCurrent();
  this->Internals->LightingHelper->PrepareForRendering();
  this->Internals->ColorMaterialHelper->PrepareForRendering();

  this->Internals->Shader->Build();
  if (this->Internals->Shader->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
  {
    vtkErrorMacro("Pass Two failed.");
    abort();
  }

  this->Internals->ColorMaterialHelper->Render();
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  this->Internals->DepthImage->Bind();

  int value = 0;
  this->Internals->Shader->GetUniformVariables()->SetUniformi("texDepth", 1, &value);
  float fvalues[2];
  fvalues[0] = static_cast<float>(viewsize[0]);
  fvalues[1] = static_cast<float>(viewsize[1]);
  this->Internals->Shader->GetUniformVariables()->SetUniformf("uViewSize", 2, fvalues);
  this->Internals->Shader->Use();
  if (!this->Internals->Shader->IsValid())
  {
    vtkErrorMacro(<< " validation of the program failed: "
                  << this->Internals->Shader->GetLastValidateLog());
  }
  this->Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);
  this->Internals->Shader->Restore();

  // Pop the attributes.
  glPopAttrib();
}

//----------------------------------------------------------------------------
void vtkVisibleLinesPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
