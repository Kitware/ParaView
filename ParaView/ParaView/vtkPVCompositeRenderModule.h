/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRenderModule.h
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
// .NAME vtkPVCompositeRenderModule - Uses composite manager and collection.
// .SECTION Description
// This render manager if for parallel execution using MPI.
// It creates a special vtkPVPartDisplay (todo) that collects small
// geometry for local rendering.  It also manages reduction factor
// which renders and composites a small window then magnifies for final
// display.

#ifndef __vtkPVCompositeRenderModule_h
#define __vtkPVCompositeRenderModule_h

#include "vtkPVLODRenderModule.h"


class VTK_EXPORT vtkPVCompositeRenderModule : public vtkPVLODRenderModule
{
public:
  static vtkPVCompositeRenderModule* New();
  vtkTypeRevisionMacro(vtkPVCompositeRenderModule,vtkPVLODRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This methods can be used from a script.  
  // "Set" sets the value of the scale, and adds an entry to the trace.
  void SetCompositeThreshold(float);
  vtkGetMacro(CompositeThreshold, float);

  // Description:
  // Renders using Still/FullRes or interactive/LODs
  virtual void StillRender();
  virtual void InteractiveRender();

  // Description:
  // Set this flag to indicate whether to calculate the reduction factor for
  // use in tree composite (or client server).
  vtkSetMacro(ReductionFactor, int);
  vtkGetMacro(ReductionFactor, int);

  // Description:
  // Squirt is a hybrid run length encoding and bit reduction compression
  // algorithm that is used to compress images for transmition from the
  // server to client.  Value of 0 disabled all compression.  Level zero is just
  // run length compression with no bit compression (lossless).
  vtkSetMacro(SquirtLevel, int);
  vtkGetMacro(SquirtLevel, int);

  // Description:
  void SetUseCompositeWithFloat(int val);
  void SetUseCompositeWithRGBA(int val);
  virtual void SetUseCompositeCompression(int val);
  
  // Description:
  // The center of rotation picker needs the compositers zbuffer.
  // Remove this method.  Change picking the center of rotation.
  vtkPVTreeComposite *GetComposite() {return this->Composite;}
  vtkGetMacro(CompositeID, vtkClientServerID);

  // Description:
  // These were originally for debugging.  I am planning on removing them !!!
  vtkGetMacro(StillCompositeTime, double);
  vtkGetMacro(InteractiveCompositeTime, double);

  // Description:
  // These use the total memory size of the visible
  // geoemtry and decimated LOD to make a collection decision.
  // I would like to move this method into a rendering module.  
  // It resides here for the moment because vtkPVWindow has a list of sources.
  int MakeCollectionDecision();
  int MakeLODCollectionDecision();

  // Description:
  // For picking the center of rotation.
  virtual float GetZBufferValue(int x, int y);

protected:
  vtkPVCompositeRenderModule();
  ~vtkPVCompositeRenderModule();

  // Subclass create their own vtkPVPartDisplay object by
  // implementing this method.
  virtual vtkPVPartDisplay* CreatePartDisplay();

  // Computes the reduction factor to use in compositing.
  void ComputeReductionFactor();
  int ReductionFactor;
  int SquirtLevel;

  int LocalRender;

  int CollectionDecision;
  int LODCollectionDecision;

  float CompositeThreshold;

  vtkPVTreeComposite *Composite;
  vtkClientServerID CompositeID;

  double StillCompositeTime;
  double InteractiveCompositeTime;

  vtkPVCompositeRenderModule(const vtkPVCompositeRenderModule&); // Not implemented
  void operator=(const vtkPVCompositeRenderModule&); // Not implemented
};


#endif
