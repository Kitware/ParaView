/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMantaManager.h"
#include "vtkManta.h"
#include "vtkObjectFactory.h"

#include <Core/Color/Color.h>
#include <Core/Color/ColorDB.h>
#include <Core/Color/RGBColor.h>
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
#include <Model/Groups/Group.h>
#include <Model/Lights/HeadLight.h>

vtkStandardNewMacro(vtkMantaManager);

//----------------------------------------------------------------------------
vtkMantaManager::vtkMantaManager()
{
  // cerr << "MX(" << this << ") CREATE" << endl;
  this->MantaEngine = Manta::createManta();
  // TODO: this requires Manta >= r2439 but I can't check that programatically
  this->MantaEngine->setDisplayBeforeRender(false);
  this->MantaFactory = new Manta::Factory(this->MantaEngine);
  this->Started = false;

  this->MantaScene = NULL;
  this->MantaWorldGroup = NULL;
  this->MantaLightSet = NULL;
  this->MantaCamera = NULL;
  this->SyncDisplay = NULL;
  this->ChannelId = 0;
}

//----------------------------------------------------------------------------
vtkMantaManager::~vtkMantaManager()
{
  // cerr << "MX(" << this << ") DESTROY" << endl;
  int v = -1;
  // TODO: This is screwy but the only way I've found to get it to consistently
  // shutdown without hanging.
  // int i = 0;
  v = this->MantaEngine->numWorkers();
  this->MantaEngine->changeNumWorkers(0);
  while (v != 0)
  {
    // cerr << "MX(" << this << ") SYNC " << i++ << " " << v << endl;
    if (this->SyncDisplay)
    {
      this->SyncDisplay->doneRendering();
      v = this->MantaEngine->numWorkers();
      if (v != 0)
      {
        this->SyncDisplay->waitOnFrameReady();
      }
    }
    v = this->MantaEngine->numWorkers();
  }
  this->MantaEngine->finish();
  this->MantaEngine->blockUntilFinished();

  // cerr << "MX(" << this << ") SYNC DONE " << i << " " << v << endl;
  // cerr << "MX(" << this << ") wait" << endl;

  if (this->MantaLightSet)
  {
    delete this->MantaLightSet->getAmbientLight();
    /*
    //let vtkMantaLight's delete themselves
    Manta::Light *light;
    for (unsigned int i = 0; i < this->MantaLightSet->numLights(); i++)
      {
      light = this->MantaLightSet->getLight(i);
      delete light;
      }
    */
  }
  delete this->MantaLightSet;

  delete this->MantaCamera;

  if (this->MantaScene)
  {
    delete this->MantaScene->getBackground();
  }
  delete this->MantaScene;

  delete this->MantaWorldGroup;

  delete this->MantaFactory;

  // delete this->SyncDisplay; //engine does this

  delete this->MantaEngine;

  // cerr << "MX(" << this << ") good night Gracie" << endl;
}

//----------------------------------------------------------------------------
void vtkMantaManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkMantaManager::StartEngine(
  int maxDepth, double* bgColor, double* ambient, bool stereo, int* size)
{
  // cerr << "MX(" << this << ") START" << endl;
  if (this->Started)
  {
    cerr << "WARNING: Manta is already initted, ignoring reinitialize." << endl;
    return;
  }
  this->Started = true;

  // create an empty Manta scene with background
  this->MantaScene = new Manta::Scene();
  this->MantaScene->getRenderParameters().setMaxDepth(maxDepth);
  this->MantaEngine->setScene(this->MantaScene);

  Manta::ConstantBackground* background = new Manta::ConstantBackground(
    Manta::Color(Manta::RGBColor(bgColor[0], bgColor[1], bgColor[2])));
  this->MantaScene->setBackground(background);

  // create empty world group
  this->MantaWorldGroup = new Manta::Group();
  this->MantaScene->setObject(this->MantaWorldGroup);

  // create empty LightSet with ambient light
  this->MantaLightSet = new Manta::LightSet();
  this->MantaLightSet->setAmbientLight(
    new Manta::ConstantAmbient(Manta::Color(Manta::RGBColor(ambient[0], ambient[1], ambient[2]))));
  this->MantaScene->setLights(this->MantaLightSet);

  // create the mantaCamera singleton,
  // it is the only camera we create per renderer
  this->MantaCamera = this->MantaFactory->createCamera("pinhole(-normalizeRays -createCornerRays)");

  // Use SyncDisplay with Null Display to stop Manta engine at each frame,
  // the image is combined with OpenGL framebuffer by vtkXMantaRenderWindow
  std::vector<std::string> vs;
  this->SyncDisplay = new Manta::SyncDisplay(vs);
  this->SyncDisplay->setChild(new Manta::NullDisplay(vs));

  // Set image size
  this->ChannelId = this->MantaEngine->createChannel(
    this->SyncDisplay, this->MantaCamera, stereo, size[0], size[1]);
}
