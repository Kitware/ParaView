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
// This class can be used alone when running serially.
// It handles the two pipeline branches which render in parallel.
// Subclasses handle parallel rendering.

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
  // In Addition to the superclass call, this method sets up
  // abort check observer on the render widnow.
  virtual void SetPVApplication(vtkPVApplication *pvApp);

  // Description:
  // This method makes the descision on whether to use LOD for rendering.
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

  // I might be able to make these private now that I moved them from 
  // RenderWindow to this class. !!!

  // Needed so to make global LOD descision.
  unsigned long GetTotalVisibleGeometryMemorySize();

protected:
  vtkPVLODRenderModule();
  ~vtkPVLODRenderModule();

  // Subclass create their own vtkPVPartDisplay object by
  // implementing this method.
  virtual vtkPVPartDisplay* CreatePartDisplay();

  // Move these to a render module when it is created.
  void ComputeTotalVisibleMemorySize();
  unsigned long TotalVisibleGeometryMemorySize;
  unsigned long TotalVisibleLODMemorySize;

  float LODThreshold;
  int LODResolution;

  unsigned long AbortCheckTag;

  vtkPVLODRenderModule(const vtkPVLODRenderModule&); // Not implemented
  void operator=(const vtkPVLODRenderModule&); // Not implemented
};


#endif
