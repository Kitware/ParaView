/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkMantaRenderer.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkManta.h"
#include "vtkMantaCamera.h"
#include "vtkMantaManager.h"
#include "vtkMantaRenderer.h"
#include "vtkMantaTexture.h"

#include "vtkActor.h"
#include "vtkCuller.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTimerLog.h"

#include <Core/Color/Color.h>
#include <Core/Color/ColorDB.h>
#include <Core/Color/RGBColor.h>
#include <Core/Geometry/Vector.h>
#include <Engine/Control/RTRT.h>
#include <Engine/Display/NullDisplay.h>
#include <Engine/Display/SyncDisplay.h>
#include <Engine/Factory/Create.h>
#include <Engine/Factory/Factory.h>
#include <Image/SimpleImage.h>
#include <Interface/Context.h>
#include <Interface/Light.h>
#include <Interface/LightSet.h>
#include <Interface/Object.h>
#include <Interface/Scene.h>
#include <Model/AmbientLights/ConstantAmbient.h>
#include <Model/Backgrounds/ConstantBackground.h>
#include <Model/Backgrounds/EnvMapBackground.h>
#include <Model/Backgrounds/LinearBackground.h>
#include <Model/Groups/Group.h>
#include <Model/Lights/HeadLight.h>

#include "vtkImageData.h"
#include "vtkPNGWriter.h"

#include <string>

vtkStandardNewMacro(vtkMantaRenderer);

//----------------------------------------------------------------------------
vtkMantaRenderer::vtkMantaRenderer()
  : EngineInited(false)
  , EngineStarted(false)
  , IsStereo(false)
  , MantaScene(0)
  , MantaWorldGroup(0)
  , MantaLightSet(0)
  , MantaCamera(0)
  , SyncDisplay(0)
{
  // cerr << "MR(" << this << ") CREATE" << endl;

  // Default options
  this->NumberOfWorkers = 1;
  this->EnableShadows = 0;
  this->Samples = 1;
  this->MaxDepth = 10;

  // the default global ambient light created by vtkRenderer is too bright.
  this->SetAmbient(0.1, 0.1, 0.1);

  this->MantaManager = vtkMantaManager::New();

  this->MantaEngine = this->MantaManager->GetMantaEngine();
  this->MantaEngine->changeNumWorkers(this->NumberOfWorkers);

  this->MantaFactory = this->MantaManager->GetMantaFactory();

  this->ColorBuffer = NULL;
  this->DepthBuffer = NULL;
  this->ImageSize = -1;

  this->MantaFactory->selectImageType("rgba8zfloat");

  this->MantaFactory->selectImageTraverser("tiled(-square)");
  // this->MantaFactory->selectImageTraverser( "deadline()" );

  this->MantaFactory->selectLoadBalancer("workqueue");

  if (this->EnableShadows)
  {
    this->MantaFactory->selectShadowAlgorithm("hard(-attenuateShadows)");
  }
  else
  {
    this->MantaFactory->selectShadowAlgorithm("noshadows");
  }

  if (this->Samples <= 1)
  {
    this->MantaFactory->selectPixelSampler("singlesample");
  }
  else
  {
    char buff[80];
    sprintf(buff, "regularsample(-numberOfSamples %d)", this->Samples);
    this->MantaFactory->selectPixelSampler(buff);
    // this->MantaFactory->selectPixelSampler(
    //"jittersample(-numberOfSamples 16)");
  }

  this->MantaFactory->selectRenderer("raytracer");

  this->DefaultLight = NULL;

  this->MantaTexture = NULL;
  this->BackgroundRight[0] = 1.0;
  this->BackgroundRight[0] = 0.0;
  this->BackgroundRight[0] = 0.0;
  this->BackgroundUp[0] = 0.0;
  this->BackgroundUp[0] = 1.0;
  this->BackgroundUp[0] = 0.0;
}

//----------------------------------------------------------------------------
vtkMantaRenderer::~vtkMantaRenderer()
{
  // cerr << "MR(" << this << ") DESTROY " << this->MantaManager << " "
  //     << this->MantaManager->GetReferenceCount() << endl;

  this->SetMantaTexture(NULL);

  if (this->DefaultLight && this->MantaLightSet)
  {
    Manta::Callback::create(this->MantaLightSet, &Manta::LightSet::remove, this->DefaultLight);
    this->DefaultLight = NULL;
  }

  this->MantaManager->Delete();

  if (this->ColorBuffer)
  {
    delete[] this->ColorBuffer;
  }
  if (this->DepthBuffer)
  {
    delete[] this->DepthBuffer;
  }
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::InitEngine()
{
  // cerr << "MR(" << this << ")#" << this->GetLayer() << " INIT "
  //     << this->MantaManager << endl;
  this->MantaManager->StartEngine(
    this->MaxDepth, this->GetBackground(), this->Ambient, this->IsStereo, this->GetSize());

  this->MantaScene = this->MantaManager->GetMantaScene();
  this->MantaWorldGroup = this->MantaManager->GetMantaWorldGroup();
  this->MantaLightSet = this->MantaManager->GetMantaLightSet();
  this->MantaCamera = this->MantaManager->GetMantaCamera();
  this->SyncDisplay = this->MantaManager->GetSyncDisplay();
  this->ChannelId = this->MantaManager->GetChannelId();

  this->EngineInited = true;
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetBackground(double r, double g, double b)
{
  if ((this->Background[0] != r) || (this->Background[1] != g) || (this->Background[2] != b))
  {
    this->Superclass::SetBackground(r, g, b);
    this->MantaEngine->addTransaction(
      "set background", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetBackground));
  };
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetBackground2(double r, double g, double b)
{
  if ((this->Background2[0] != r) || (this->Background2[1] != g) || (this->Background2[2] != b))
  {
    this->Superclass::SetBackground2(r, g, b);
    this->MantaEngine->addTransaction(
      "set background", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetBackground));
  };
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetGradientBackground(bool val)
{
  if (this->GetGradientBackground() != val)
  {
    this->Superclass::SetGradientBackground(val);
    this->MantaEngine->addTransaction(
      "set background", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetBackground));
  };
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetBackgroundTexture(vtkTexture* texture)
{
  if (this->GetBackgroundTexture() != texture)
  {
    this->Superclass::SetBackgroundTexture(texture);

    if (texture)
    {
      vtkMantaTexture* mt = vtkMantaTexture::New();
      mt->SetInputData(this->GetBackgroundTexture()->GetInput());
      this->SetMantaTexture(mt);
      mt->Delete();
    }
    else
    {
      this->SetMantaTexture(NULL);
    }

    this->MantaEngine->addTransaction(
      "set background", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetBackground));
  };
}

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkMantaRenderer, MantaTexture, vtkMantaTexture);

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetTexturedBackground(bool val)
{
  if (this->GetTexturedBackground() != val)
  {
    this->Superclass::SetTexturedBackground(val);
    this->MantaEngine->addTransaction(
      "set background", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetBackground));
  };
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetBackgroundUp(double x, double y, double z)
{
  if ((this->BackgroundUp[0] != x) || (this->BackgroundUp[1] != y) || (this->BackgroundUp[2] != z))
  {
    this->BackgroundUp[0] = x;
    this->BackgroundUp[1] = y;
    this->BackgroundUp[2] = z;
    this->Modified();
    this->MantaEngine->addTransaction(
      "set background", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetBackground));
  };
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetBackgroundRight(double x, double y, double z)
{
  if ((this->BackgroundRight[0] != x) || (this->BackgroundRight[1] != y) ||
    (this->BackgroundRight[2] != z))
  {
    this->BackgroundRight[0] = x;
    this->BackgroundRight[1] = y;
    this->BackgroundRight[2] = z;
    this->Modified();
    this->MantaEngine->addTransaction(
      "set background", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetBackground));
  };
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::InternalSetBackground()
{
  if (this->GetTexturedBackground() && this->GetMantaTexture())
  {
    vtkMantaTexture* mt = this->GetMantaTexture();
    mt->Load(this);
    double* up = this->GetBackgroundUp();
    double* right = this->GetBackgroundRight();
    Manta::EnvMapBackground* background =
      new Manta::EnvMapBackground(mt->GetMantaTexture(), Manta::EnvMapBackground::LatLon,
        Manta::Vector(right[0], right[1], right[2]), Manta::Vector(up[0], up[1], up[2]));
    delete this->MantaScene->getBackground();
    this->MantaScene->setBackground(background);
  }
  else if (this->GetGradientBackground())
  {
    double* color1 = this->GetBackground();
    double* color2 = this->GetBackground2();
    double* up = this->GetBackgroundUp();
    Manta::LinearBackground* background =
      new Manta::LinearBackground(Manta::Color(Manta::RGBColor(color1[0], color1[1], color1[2])),
        Manta::Color(Manta::RGBColor(color2[0], color2[1], color2[2])),
        Manta::Vector(up[0], up[1], up[2]));
    delete this->MantaScene->getBackground();
    this->MantaScene->setBackground(background);
  }
  else
  {
    double* color1 = this->GetBackground();
    Manta::ConstantBackground* background =
      new Manta::ConstantBackground(Manta::Color(Manta::RGBColor(color1[0], color1[1], color1[2])));
    delete this->MantaScene->getBackground();
    this->MantaScene->setBackground(background);
  }
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::ClearLights(void)
{
  this->MantaEngine->addTransaction(
    "clear lights", Manta::Callback::create(this, &vtkMantaRenderer::InternalClearLights));
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::InternalClearLights(void)
{
  if (this->MantaLightSet)
  {
    delete this->MantaLightSet->getAmbientLight();
    for (unsigned int i = 0; i < this->MantaLightSet->numLights(); i++)
    {
      Manta::Light* light = this->MantaLightSet->getLight(i);
      this->MantaLightSet->remove(light);
      delete light;
    }
  }
}

//----------------------------------------------------------------------------
// Ask lights to load themselves into graphics pipeline.
int vtkMantaRenderer::UpdateLights()
{
  // convert VTK lights into Manta lights
  vtkCollectionSimpleIterator sit;
  this->Lights->InitTraversal(sit);

  // TODO: schedule ClearLight here?
  vtkLight* vLight = NULL;
  bool noneOn = true;
  for (this->Lights->InitTraversal(sit); (vLight = this->Lights->GetNextLight(sit));)
  {
    if (vLight->GetSwitch())
    {
      noneOn = false;
    }
    // manta lights set intensity to 0.0 if switched off, so render regardless
    vLight->Render(this, 0 /* not used */);
  }

  if (noneOn)
  {
    if (this->MantaLightSet->numLights() == 0)
    {
      // there is no VTK light nor MantaLight defined, create a Manta headlight
      cerr << "No light defined, creating a headlight at camera position" << endl;
      this->DefaultLight = new Manta::HeadLight(0, Manta::Color(Manta::RGBColor(1, 1, 1)));
      this->MantaEngine->addTransaction("add headlight",
        Manta::Callback::create(this->MantaLightSet, &Manta::LightSet::add, this->DefaultLight));
    }
  }
  else
  {
    if (this->DefaultLight)
    {
      Manta::Callback::create(this->MantaLightSet, &Manta::LightSet::remove, this->DefaultLight);
      this->DefaultLight = NULL;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
vtkCamera* vtkMantaRenderer::MakeCamera()
{
  return vtkMantaCamera::New();
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::UpdateSize()
{
  if (this->EngineStarted)
  {
    int mantaSize[2];
    int* renderSize = NULL;
    bool stereoDummyArg;

    renderSize = this->GetSize();
    this->GetSyncDisplay()->getCurrentImage()->getResolution(
      stereoDummyArg, mantaSize[0], mantaSize[1]);

    if (mantaSize[0] != renderSize[0] || mantaSize[1] != renderSize[1])
    {
      /*
      cerr << "MR(" << this << ") "
           << "Layer: " << this->GetLayer() << ", "
           << "Props: " << this->NumberOfPropsRendered << endl
           << "  MantaSize: " << mantaSize[0] << ", " << mantaSize[1] << ", "
           << "  renderSize: " << renderSize[0] << ", " << renderSize[1] << endl;
      */
      this->GetMantaEngine()->addTransaction("resize",
        Manta::Callback::create(this->GetMantaEngine(), &Manta::MantaInterface::changeResolution, 0,
                                               renderSize[0], renderSize[1], true));

      this->GetSyncDisplay()->doneRendering();
      this->GetSyncDisplay()->waitOnFrameReady();
    }
  }
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::DeviceRender()
{
  // cerr << "MR(" << this << ") DeviceRender" << endl;

  // In ParaView, we are wasting time in rendering the "sync layer" with
  // empty background image just to be dropped in LayerRender(). We just
  // don't start the engine with sync layer.
  // TODO: this may not be the right way to check if it is a sync layer
  if (this->GetLayer() != 0 && this->GetActors()->GetNumberOfItems() == 0)
  {
    return;
  }

  vtkTimerLog::MarkStartEvent("Manta Dev Render");

  if (!this->EngineInited)
  {
    this->InitEngine();
  }

  vtkTimerLog::MarkStartEvent("Geometry");

  this->Clear();

  this->UpdateSize();

  // call camera::Render()
  this->UpdateCamera();

  // TODO: call ClearLights here?

  // call Light::Render()
  this->UpdateLightGeometry();
  this->UpdateLights();

  if (!this->EngineStarted)
  {
    this->MantaEngine->beginRendering(false);
    this->EngineStarted = true;
    this->GetSyncDisplay()->waitOnFrameReady();
  }

  this->UpdateGeometry();

  vtkTimerLog::MarkEndEvent("Geometry");

  vtkTimerLog::MarkStartEvent("Total LayerRender");
  this->LayerRender();
  vtkTimerLog::MarkEndEvent("Total LayerRender");

  vtkTimerLog::MarkEndEvent("Manta Dev Render");
}

//----------------------------------------------------------------------------
// let the renderer display itself appropriately based on its layer index
void vtkMantaRenderer::LayerRender()
{
  // TODO:
  // this needs to be simplified. Now that UpdateSize happens before this
  // vtk's size and manta's size should always be the same so the ugly
  // conversion and minsize safety check should go away
  int i, j;
  int rowLength, mantaSize[2];
  int minWidth, minHeight;
  int hMantaDiff, hRenderDiff;
  int renderPos0[2];
  int* renderSize = NULL;
  int* renWinSize = NULL;
  bool stereoDumy;
  float* mantaBuffer = NULL;
  double* renViewport = NULL;
  const Manta::SimpleImageBase* mantaBase = NULL;

  // collect some useful info
  renderSize = this->GetSize();
  renWinSize = this->GetRenderWindow()->GetActualSize();
  renViewport = this->GetViewport();
  renderPos0[0] = int(renViewport[0] * renWinSize[0] + 0.5f);
  renderPos0[1] = int(renViewport[1] * renWinSize[1] + 0.5f);

  // get render threads to draw what we've asked them to
  vtkTimerLog::MarkStartEvent("ThreadSync");
  this->GetSyncDisplay()->doneRendering();
  // now transaction callbacks happen
  this->GetSyncDisplay()->waitOnFrameReady();
  // now we have frame n-1's image
  this->GetSyncDisplay()->doneRendering();
  // now transaction callback effects take place
  this->GetSyncDisplay()->waitOnFrameReady();
  // now we have frame n's image
  vtkTimerLog::MarkEndEvent("ThreadSync");

  this->GetSyncDisplay()->getCurrentImage()->getResolution(stereoDumy, mantaSize[0], mantaSize[1]);
  mantaBase =
    dynamic_cast<const Manta::SimpleImageBase*>(this->GetSyncDisplay()->getCurrentImage());
  rowLength = mantaBase->getRowLength();

  // for window re-sizing
  minWidth = (mantaSize[0] < renderSize[0]) ? mantaSize[0] : renderSize[0];
  minHeight = (mantaSize[1] < renderSize[1]) ? mantaSize[1] : renderSize[1];
  hMantaDiff = mantaSize[1] - minHeight;
  hRenderDiff = renderSize[1] - minHeight;

  // memory allocation and acess to the Manta image
  int size = renderSize[0] * renderSize[1];
  if (this->ImageSize != size)
  {
    delete[] this->ColorBuffer;
    delete[] this->DepthBuffer;
    this->ImageSize = size;
    this->DepthBuffer = new float[size];
    this->ColorBuffer = new float[size];
  }
  mantaBuffer = static_cast<float*>(mantaBase->getRawData(0));

  // update this->ColorBuffer and this->DepthBuffer from the Manta
  // RGBA8ZfloatPixel array
  double* clipValues = this->GetActiveCamera()->GetClippingRange();
  double depthScale = 1.0f / (clipValues[1] - clipValues[0]);

  vtkTimerLog::MarkStartEvent("Image Conversion");
  for (j = 0; j < minHeight; j++)
  {
    // there are two floats in each pixel in Manta buffer
    int mantaIndex = ((j + hMantaDiff) * rowLength) * 2;
    // there is only one float in each pixel in the GL RGBA or Z buffer
    int tupleIndex = ((j + hRenderDiff) * renderSize[0]) * 1;

    for (i = 0; i < minWidth; i++)
    {
      this->ColorBuffer[tupleIndex + i] = mantaBuffer[mantaIndex + i * 2];
      float depthValue = mantaBuffer[mantaIndex + i * 2 + 1];
      // normalize the depth values to [ 0.0f, 1.0f ], since we are using a
      // software buffer for Z values and never write them to OpenGL buffers,
      // we don't have to clamp them any more
      // TODO: On a second thought, we probably don't even have to normalize Z
      // values at all
      this->DepthBuffer[tupleIndex + i] = (depthValue - clipValues[0]) * depthScale;
    }
  }

  // let layer #0 initialize GL depth buffer
  if (this->GetLayer() == 0)
  {
    this->GetRenderWindow()->SetZbufferData(renderPos0[0], renderPos0[1],
      renderPos0[0] + renderSize[0] - 1, renderPos0[1] + renderSize[1] - 1, this->DepthBuffer);

    this->GetRenderWindow()->SetRGBACharPixelData(renderPos0[0], renderPos0[1],
      renderPos0[0] + renderSize[0] - 1, renderPos0[1] + renderSize[1] - 1,
      (unsigned char*)this->ColorBuffer, 0, 0);
    glFinish();
  }
  else
  {
    // layers on top add the colors of their non background pixels
    unsigned char* GLbakBuffer = NULL;
    GLbakBuffer = this->GetRenderWindow()->GetRGBACharPixelData(renderPos0[0], renderPos0[1],
      renderPos0[0] + renderSize[0] - 1, renderPos0[1] + renderSize[1] - 1, 0);

    bool anyhit = false;
    unsigned char* optr = GLbakBuffer;
    unsigned char* iptr = (unsigned char*)this->ColorBuffer;
    float* zptr = this->DepthBuffer;
    for (j = 0; j < renderSize[1]; j++)
    {
      for (i = 0; i < renderSize[0]; i++)
      {
        const float z = *zptr;
        if (z > 0 && z < 1.0)
        {
          anyhit = true;
          *(optr + 0) = *(iptr + 0);
          *(optr + 1) = *(iptr + 1);
          *(optr + 2) = *(iptr + 2);
          *(optr + 3) = *(iptr + 3);
        }
        optr += 4;
        iptr += 4;
        zptr++;
      }
    }

    if (anyhit)
    {
      // submit the modified RGB colors to GL BACK buffer
      this->GetRenderWindow()->SetRGBACharPixelData(renderPos0[0], renderPos0[1],
        renderPos0[0] + renderSize[0] - 1, renderPos0[1] + renderSize[1] - 1, GLbakBuffer, 0, 0);
    }

    delete[] GLbakBuffer;
  }

  // cerr << "MR(" << this << ") release" << endl;
  vtkTimerLog::MarkEndEvent("Image Conversion");
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetNumberOfWorkers(int newval)
{
  if (this->NumberOfWorkers == newval)
  {
    return;
  }
  this->NumberOfWorkers = newval;
  this->MantaEngine->addTransaction(
    "set max depth", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetNumberOfWorkers));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::InternalSetNumberOfWorkers()
{
  this->MantaEngine->changeNumWorkers(this->NumberOfWorkers);
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetEnableShadows(int newval)
{
  if (this->EnableShadows == newval)
  {
    return;
  }

  this->EnableShadows = newval;
  this->MantaEngine->addTransaction(
    "set shadows", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetShadows));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::InternalSetShadows()
{
  if (this->EnableShadows)
  {
    this->MantaFactory->selectShadowAlgorithm("hard(-attenuateShadows)");
  }
  else
  {
    this->MantaFactory->selectShadowAlgorithm("noshadows");
  }
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetSamples(int newval)
{
  if (this->Samples == newval)
  {
    return;
  }

  this->Samples = newval;
  this->MantaEngine->addTransaction(
    "set samples", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetSamples));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::InternalSetSamples()
{
  if (this->Samples <= 1)
  {
    this->MantaFactory->selectPixelSampler("singlesample");
  }
  else
  {
    char buff[80];
    sprintf(buff, "regularsample(-numberOfSamples %d)", this->Samples);
    this->MantaFactory->selectPixelSampler(buff);
    // this->MantaFactory->selectPixelSampler(
    //"jittersample(-numberOfSamples 16)");
  }
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::SetMaxDepth(int newval)
{
  if (this->MaxDepth == newval)
  {
    return;
  }

  this->MaxDepth = newval;
  this->MantaEngine->addTransaction(
    "set max depth", Manta::Callback::create(this, &vtkMantaRenderer::InternalSetMaxDepth));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMantaRenderer::InternalSetMaxDepth()
{
  this->MantaScene->getRenderParameters().setMaxDepth(this->MaxDepth);
}
