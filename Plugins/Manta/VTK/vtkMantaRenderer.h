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
// .NAME vtkMantaRenderer - Manta renderer
// .SECTION Description
// vtkMantaRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkMantaRenderer interfaces to the Manta graphics library.

#ifndef __vtkMantaRenderer_h
#define __vtkMantaRenderer_h

#include "vtkMantaConfigure.h"
#include "vtkRenderer.h"

//BTX
namespace Manta {
class MantaInterface;
class Scene;
class Object;
class Group;
class LightSet;
class Factory;
class Camera;
class SyncDisplay;
}
//ETX

class vtkPKdTree;
typedef vtkTypeUInt32 uint32;

class VTK_vtkManta_EXPORT vtkMantaRenderer : public vtkRenderer
{
public:
  static vtkMantaRenderer *New();
  vtkTypeRevisionMacro(vtkMantaRenderer,vtkRenderer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Concrete open gl render method.
  void DeviceRender();

  // Description:
  // Internal method temporarily removes lights before reloading them
  // into graphics pipeline.
  void ClearLights(void);

  // Description:
  // Ask lights to load themselves into graphics pipeline.
  int UpdateLights(void);

  //BTX
  // Regular non-vtkSetGet functions are used here as Manta does NOT
  // adopt reference-counting to manage objects. Otherwise it would
  // complicate the memory management issue due to the inconsistency.
  void   SetMantaEngine( Manta::MantaInterface * engine )
         { this->MantaEngine = engine; }
  void   SetMantaFactory( Manta::Factory * factory )
         { this->MantaFactory = factory; }
  void   SetMantaScene( Manta::Scene * scene )
         { this->MantaScene = scene; }
  void   SetMantaWorldGroup( Manta::Group * group )
         { this->MantaWorldGroup = group; }
  void   SetMantaLightSet( Manta::LightSet * lightSet )
         { this->MantaLightSet = lightSet; }
  void   SetMantaCamera( Manta::Camera * camera )
         { this->MantaCamera = camera; }
  void   SetSyncDisplay( Manta::SyncDisplay * syncDisplay )
         { this->SyncDisplay = syncDisplay; }
  Manta::MantaInterface * GetMantaEngine()    { return this->MantaEngine;    }
  Manta::Factory        * GetMantaFactory()   { return this->MantaFactory;   }
  Manta::Scene          * GetMantaScene()     { return this->MantaScene;     }
  Manta::Group          * GetMantaWorldGroup(){ return this->MantaWorldGroup;}
  Manta::LightSet       * GetMantaLightSet()  { return this->MantaLightSet;  }
  Manta::Camera         * GetMantaCamera()    { return this->MantaCamera;    }
  Manta::SyncDisplay    * GetSyncDisplay()    { return this->SyncDisplay;    }
  float * GetColorBuffer() { return this->ColorBuffer; }
  float * GetDepthBuffer() { return this->DepthBuffer; }
  //ETX

//  vtkGetMacro(LayerBuffer, float*);
//  vtkGetMacro(DepthBuffer, float*);
  vtkSetMacro(ChannelId, int);
  vtkGetMacro(ChannelId, int);
  void ChangeNumberOfWorkers(int numWorkers);

protected:
  vtkMantaRenderer();
  ~vtkMantaRenderer();

  // Picking functions to be implemented by sub-classes
  virtual void DevicePickRender() {};
  virtual void StartPick(unsigned int pickFromSize) {};
  virtual void UpdatePickId() {};
  virtual void DonePick() {};
  virtual unsigned int GetPickedId() { return 0; };
  virtual unsigned int GetNumPickedIds() { return 0; };
  virtual int GetPickedIds( unsigned int atMost, unsigned int * callerBuffer )
    { return 0; };
  virtual double GetPickedZ() { return 0.0f; };

private:
  vtkMantaRenderer(const vtkMantaRenderer&); // Not implemented.
  void operator=(const vtkMantaRenderer&); // Not implemented.

  void InitEngine();
  void LayerRender();

  // called right before UpdateGeometry()
  void UpdateActorsForVisibility();

  // orverrides the counterpart of the parent class, i.e., vtkRenderer, that
  // always creates a vtkOpenGLCamera object, as governed by a startdard
  // object factory in vtkGraphicsFactory, despite the explicit specification
  // of vtkMantaCamera as the expected concrete class type in the server XML
  // file. Lack of this overriding function would disable vtkManta plug-ins to
  // work in parallel mode.
  vtkCamera * MakeCamera();

  int  MaxDepth;
  int  NumberOfWorkers;
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
  //ETX
  int ChannelId;
};

#endif
