/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODRenderModule.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVLODRenderModule - Mangages rendering and LODs.
// .SECTION Description
// This is currently the only type of render module.  I intend to split
// it up into serial, parallel, client-server, and tile-display objects.


#ifndef __vtkPVLODRenderModule_h
#define __vtkPVLODRenderModule_h

#include "vtkPVRenderModule.h"

class vtkPVTreeComposite;

class VTK_EXPORT vtkPVLODRenderModule : public vtkPVRenderModule
{
public:
  static vtkPVLODRenderModule* New();
  vtkTypeRevisionMacro(vtkPVLODRenderModule,vtkPVRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the application right after construction.
  virtual void SetPVApplication(vtkPVApplication *pvApp);

  // Description:
  // Call StillRender when you want to use the full resolution render.
  // InteractiveRender called by the interactor styles.
  virtual void StillRender();
  virtual void InteractiveRender();

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  void SetLODThreshold(float);
  vtkGetMacro(LODThreshold, float);

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  void SetLODResolution(int);
  vtkGetMacro(LODResolution, int);

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  void SetCollectThreshold(float);
  vtkGetMacro(CollectThreshold, float);

  // Description:
  // Set this flag to indicate whether to calculate the reduction factor for
  // use in tree composite.
  vtkSetMacro(UseReductionFactor, int);
  vtkGetMacro(UseReductionFactor, int);
  vtkBooleanMacro(UseReductionFactor, int);

  // Description:
  void SetUseCompositeWithFloat(int val);
  void SetUseCompositeWithRGBA(int val);
  void SetUseCompositeCompression(int val);
  
  // Description:
  // The center of rotation picker needs the compositers zbuffer.
  // Remove this method.  Change picking the center of rotation.
  vtkPVTreeComposite *GetComposite() {return this->Composite;}
  vtkGetStringMacro(CompositeTclName);

  // Description:
  // These were originally for debugging.  I am planning on removing them !!!
  vtkGetMacro(StillCompositeTime, double);
  vtkGetMacro(InteractiveCompositeTime, double);

  // I might be able to make these private now that I moved them from 
  // RenderWindow to this class. !!!

  // Description:
  // These use the total memory size of the visible
  // geoemtry and decimated LOD to make a collection decision.
  // I would like to move this method into a rendering module.  
  // It resides here for the moment because vtkPVWindow has a list of sources.
  int MakeCollectionDecision();
  int MakeLODCollectionDecision();
  // Needed so to make global LOD descision.
  unsigned long GetTotalVisibleGeometryMemorySize();

protected:
  vtkPVLODRenderModule();
  ~vtkPVLODRenderModule();

  // Computes the reduction factor to use in compositing.
  void StartRender();

  // This is used before a render to make sure all visible sources
  // have been updated.  It returns 1 if all the data has been collected
  // and the render should be local. A side action is to set the Global LOD
  // Flag.  This is what the argument is used for.  I do not think this is 
  // best place to do this ...
  virtual int UpdateAllPVData(int interactive);

  // Move these to a render module when it is created.
  void ComputeTotalVisibleMemorySize();
  unsigned long TotalVisibleGeometryMemorySize;
  unsigned long TotalVisibleLODMemorySize;
  int CollectionDecision;
  int LODCollectionDecision;

  float LODThreshold;
  int LODResolution;
  float CollectThreshold;
  int UseReductionFactor;

  vtkPVTreeComposite *Composite;
  char *CompositeTclName;
  vtkSetStringMacro(CompositeTclName);

  unsigned long AbortCheckTag;

  double StillCompositeTime;
  double InteractiveCompositeTime;

  vtkPVLODRenderModule(const vtkPVLODRenderModule&); // Not implemented
  void operator=(const vtkPVLODRenderModule&); // Not implemented
};


#endif
