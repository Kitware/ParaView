/*=========================================================================

   Program: ParaView
   Module: vtkShaderBallScene.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/

#include "vtkShaderBallScene.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLSkybox.h"
#include "vtkPNGReader.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTexture.h"
#include "vtkXMLImageDataWriter.h"

#include "ShaderBallSkybox.h"  // Expose the jpg buffer of the skybox as 'ShaderBallSkybox'
#include "ShaderBallTexture.h" // Expose the png buffer of the plane texture as 'ShaderBallTexture'

namespace details
{
void AddCurvedPlane(vtkRenderer* renderer)
{
  vtkNew<vtkPlaneSource> plane;
  const double size = 3.5;
  const double yPos = -0.5;
  plane->SetOrigin(-size, yPos, size);
  plane->SetPoint1(size, yPos, size);
  plane->SetPoint2(-size, yPos, -1.8 * size);
  plane->SetNormal(0., 1., 0.);
  plane->SetResolution(2, 130);
  plane->Update();

  vtkPolyData* planeData = plane->GetOutput();

  vtkFloatArray* tcoord = vtkFloatArray::SafeDownCast(planeData->GetPointData()->GetTCoords());
  for (vtkIdType i = 0; i < tcoord->GetNumberOfTuples(); ++i)
  {
    float tmp[2];
    tcoord->GetTypedTuple(i, tmp);
    for (vtkIdType j = 0; j < 2; ++j)
    {
      tmp[j] = 5. * (2. * tmp[j] - 1.);
    }

    tcoord->SetTuple2(i, tmp[0], tmp[1]);
  }

  vtkFloatArray* position = vtkFloatArray::SafeDownCast(planeData->GetPoints()->GetData());
  for (vtkIdType i = 0; i < position->GetNumberOfTuples(); ++i)
  {
    float tmp[3];
    position->GetTypedTuple(i, tmp);
    const float x = tmp[2];
    if (x < 0.f)
    {
      tmp[1] = 0.005 * std::exp(-x) + yPos;
    }
    position->SetTuple3(i, tmp[0], tmp[1], tmp[2]);
  }

  vtkNew<vtkPolyDataMapper> planeMapper;
  planeMapper->SetInputData(planeData);

  vtkNew<vtkActor> planeActor;
  planeActor->SetMapper(planeMapper);

  vtkNew<vtkPNGReader> reader;
  reader->SetMemoryBuffer(ShaderBallTexture);
  reader->SetMemoryBufferLength(sizeof(ShaderBallTexture));
  reader->Update();

  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->InterpolateOn();
  texture->SetWrap(vtkTexture::Repeat);
  texture->SetMipmap(true);
  texture->SetInputData(reader->GetOutput());

  planeActor->SetTexture(texture);

  renderer->AddActor(planeActor);
}

void AddSkyBox(vtkRenderer* renderer)
{
  vtkNew<vtkJPEGReader> reader;
  reader->SetMemoryBuffer(ShaderBallSkybox);
  reader->SetMemoryBufferLength(sizeof(ShaderBallSkybox));
  reader->Update();

  vtkNew<vtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->InterpolateOn();
  texture->SetMipmap(true);
  texture->SetInputData(reader->GetOutput());

  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(texture);

  vtkNew<vtkOpenGLSkybox> skybox;
  skybox->SetFloorRight(0.0, 0.0, 1.0);
  skybox->SetProjection(vtkSkybox::Sphere);
  skybox->SetTexture(texture);

  renderer->AddActor(skybox);
}
}

vtkStandardNewMacro(vtkShaderBallScene);

//-----------------------------------------------------------------------------
vtkShaderBallScene::vtkShaderBallScene()
{
  this->Renderer->SetRenderWindow(this->Window);

  this->ResetPass();

  vtkOSPRayRendererNode::SetSamplesPerPixel(4, this->Renderer);
  vtkOSPRayRendererNode::SetRendererType("pathtracer", this->Renderer);
  vtkOSPRayRendererNode::SetRouletteDepth(5, this->Renderer);
  vtkOSPRayRendererNode::SetEnableDenoiser(1, this->Renderer);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(40);
  sphere->SetThetaResolution(40);

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  this->SphereActor->SetMapper(sphereMapper);
  this->SphereActor->GetProperty()->SetInterpolationToPBR();

  this->Renderer->AddActor(this->SphereActor);

  details::AddCurvedPlane(this->Renderer);
  details::AddSkyBox(this->Renderer);

  vtkCamera* camera = this->Renderer->GetActiveCamera();
  camera->Elevation(15.);
  camera->Dolly(0.25);

  this->Renderer->RemoveAllLights();
  this->Renderer->AutomaticLightCreationOff();

  this->Window->AddRenderer(this->Renderer);
}

//-----------------------------------------------------------------------------
vtkShaderBallScene::~vtkShaderBallScene()
{
  if (this->OSPRayPass)
  {
    this->OSPRayPass->Delete();
  }
}

//-----------------------------------------------------------------------------
void vtkShaderBallScene::Render()
{
  if (this->Visible && this->NeedRender)
  {
    this->Window->Render();
    this->NeedRender = false;
  }
}

//-----------------------------------------------------------------------------
void vtkShaderBallScene::SetMaterialName(const char* materialName)
{
  if (!materialName || !*materialName)
  {
    return;
  }

  if (!this->SphereActor->GetProperty()->GetMaterialName() ||
    strcmp(materialName, this->SphereActor->GetProperty()->GetMaterialName()) != 0)
  {
    this->SphereActor->GetProperty()->SetMaterialName(materialName);
    this->NeedRender = true;
  }
}

//-----------------------------------------------------------------------------
void vtkShaderBallScene::Modified()
{
  this->Superclass::Modified();
  // This is to ensure that we pass new properties to the sphere
  this->SphereActor->GetProperty()->Modified();
  this->NeedRender = true;
  this->Render();
}

//-----------------------------------------------------------------------------
void vtkShaderBallScene::ResetPass()
{
  if (this->OSPRayPass)
  {
    this->OSPRayPass->Delete();
  }

  this->OSPRayPass = vtkOSPRayPass::New();
  this->Renderer->SetPass(this->OSPRayPass);

  this->NeedRender = true;
  this->Render();
}

//-----------------------------------------------------------------------------
void vtkShaderBallScene::SetNumberOfSamples(int numberOfSamples)
{
  vtkOSPRayRendererNode::SetSamplesPerPixel(numberOfSamples, this->Renderer);
  this->NeedRender = true;
  this->Render();
}

//-----------------------------------------------------------------------------
void vtkShaderBallScene::SetVisible(bool visible)
{
  if (this->Visible == visible)
  {
    return;
  }
  this->Visible = visible;
  if (this->Visible)
  {
    this->NeedRender = true;
    this->Render();
  }
}

//----------------------------------------------------------------------------
void vtkShaderBallScene::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NeedRender: " << this->NeedRender << endl;
  os << indent << "Visible: " << this->Visible << endl;
}
