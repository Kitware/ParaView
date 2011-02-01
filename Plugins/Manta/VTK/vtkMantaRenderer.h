/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkMantaRenderer.h

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
// .NAME vtkMantaRenderer - Renderer that uses Manta ray tracer instead of GL.
// .SECTION Description
// vtkMantaRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkMantaRenderer interfaces to the Manta graphics library.

#ifndef __vtkMantaRenderer_h
#define __vtkMantaRenderer_h

#include "vtkMantaConfigure.h"
#include "vtkOpenGLRenderer.h"

//BTX
namespace Manta {
class MantaInterface;
class Scene;
class Group;
class LightSet;
class Factory;
class Camera;
class SyncDisplay;
class Light;
}
//ETX

class vtkMantaManager;

class VTK_vtkManta_EXPORT vtkMantaRenderer : public vtkOpenGLRenderer
{
public:
  static vtkMantaRenderer *New();
  vtkTypeMacro(vtkMantaRenderer,vtkRenderer);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  // Overridden to use manta callbacks to do the work.
  virtual void SetBackground(double r, double g, double b);

  //Description:
  //Changes the number of manta rendering threads.
  //More is faster.
  //Default is 1.
  void SetNumberOfWorkers(int);
  vtkGetMacro(NumberOfWorkers, int);

  //Description:
  //Turns on or off shadow rendering.
  //Default is off.
  void SetEnableShadows(int);
  vtkGetMacro(EnableShadows, int);

  //Description:
  //Controls multisample (anitaliased) rendering. 
  //More looks better, but is slower.
  //Default is 1.
  void SetSamples(int);
  vtkGetMacro(Samples, int);

  //Description:
  //Controls maximum ray bounce depth.
  //More looks better, but is slower.
  //Default is 5 meaning a couple of bounces.
  void SetMaxDepth(int);
  vtkGetMacro(MaxDepth, int);

  // Description:
  // Ask lights to load themselves into graphics pipeline.
  // TODO: is this necessary?
  int UpdateLights(void);

  // Description:
  // Turns off all lighting
  // TODO: is this necessary?
  void ClearLights(void);

  //Description:
  //Access to the manta rendered image
  float * GetColorBuffer() 
  { 
    return this->ColorBuffer; 
  }
  float * GetDepthBuffer() 
  {
    return this->DepthBuffer; 
  }

  // Description:
  // Concrete render method. Do not call this directly. The pipeline calls
  // it during Renderwindow::Render()
  void DeviceRender();

  //Description:
  //All classes that make manta calls should get hold of this and
  //Register it so that the Manager, and thus the manta engine
  //outlive themselves, and thus guarantee that they can safely make
  //manta API calls whenever they need to.
  vtkMantaManager* GetMantaManager()
  {
    return this->MantaManager;
  }

  //BTX
  //Description:
  //Convenience read accessors to Manta structures
  Manta::MantaInterface* GetMantaEngine()    
  { 
  return this->MantaEngine;
  }
  Manta::Factory* GetMantaFactory()
  { 
    return this->MantaFactory;
  }
  Manta::Scene* GetMantaScene() 
  { 
    return this->MantaScene;
  }
  Manta::Group* GetMantaWorldGroup() 
  {
    return this->MantaWorldGroup;
  }
  Manta::LightSet* GetMantaLightSet() 
  {
    return this->MantaLightSet;
  }
  Manta::Camera* GetMantaCamera()
  { 
    return this->MantaCamera;
  }
  Manta::SyncDisplay* GetSyncDisplay() 
  {
    return this->SyncDisplay;
  }
  //ETX

  //Description:
  //Internal callbacks for manta thread use.
  //Do not call them directly.
  void InternalSetBackground();
  void InternalClearLights();
  void InternalSetNumberOfWorkers();
  void InternalSetShadows();
  void InternalSetSamples();
  void InternalSetMaxDepth();

protected:
  vtkMantaRenderer();
  ~vtkMantaRenderer();

  //lets manta engine know when viewport changes
  void UpdateSize();

  // Manta renderer does not support picking.
  virtual void DevicePickRender() {};
  virtual void StartPick(unsigned int pickFromSize) {};
  virtual void UpdatePickId() {};
  virtual void DonePick() {};
  virtual unsigned int GetPickedId() { return 0; };
  virtual unsigned int GetNumPickedIds() { return 0; };
  virtual int GetPickedIds( unsigned int atMost, unsigned int * callerBuffer )
  {
    return 0; 
  };
  virtual double GetPickedZ() { return 0.0f; };

private:
  vtkMantaRenderer(const vtkMantaRenderer&); // Not implemented.
  void operator=(const vtkMantaRenderer&); // Not implemented.

  void InitEngine();
  void LayerRender();

  //Description:
  // Overriden to help ensure that a Manta compatible class is created.
  vtkCamera * MakeCamera();

  bool IsStereo;
  bool EngineInited;
  bool EngineStarted;

  int ImageSize;
  float *ColorBuffer;
  float *DepthBuffer;

  //BTX
  Manta::MantaInterface * MantaEngine;
  Manta::Factory * MantaFactory;
  Manta::Scene * MantaScene;
  Manta::Group * MantaWorldGroup;
  Manta::LightSet * MantaLightSet;
  Manta::Camera * MantaCamera;
  Manta::SyncDisplay * SyncDisplay;
  Manta::Light * DefaultLight;
  //ETX

  int ChannelId;

  vtkMantaManager *MantaManager;

  int NumberOfWorkers;
  int EnableShadows;
  int Samples;
  int MaxDepth;
};

#endif
