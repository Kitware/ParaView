/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModule.h
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
// .NAME vtkPVRenderModule - Mangages rendering and displaying data.
// .SECTION Description

// I am in the process of moving features around into new objects.
// This is a super class for all rendering modules.
// Subclasses manages creation and manipulation of the render window.  
// No user interface in this class.
// Some common camera manipulation (and other render window control)
// may be moved back into vtkPVRenderView to simplify this module.

// Although I do not intend that this class should be instantiated
// and used as a rendering module, I am implementing the methods
// in the most simple way.  No LODs or parallel support.

#ifndef __vtkPVRenderModule_h
#define __vtkPVRenderModule_h

#include "vtkObject.h"

class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVData;
class vtkPVSource;
class vtkPVSourceList;
class vtkPVWindow;
class vtkRenderer;
class vtkRenderWindow;
class vtkCollection;

class VTK_EXPORT vtkPVRenderModule : public vtkObject
{
public:
  static vtkPVRenderModule* New();
  vtkTypeRevisionMacro(vtkPVRenderModule,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the application right after construction.
  virtual void SetPVApplication(vtkPVApplication *pvApp);
  vtkGetObjectMacro(PVApplication, vtkPVApplication);

  // Description:
  // Compute the bounding box of all the visibile props
  // Used in ResetCamera() and ResetCameraClippingRange()
  void ComputeVisiblePropBounds( float bounds[6] ); 
  

  // Description:
  // This method is executed in all processes.
  void AddPVSource(vtkPVSource *pvs);
  void RemovePVSource(vtkPVSource *pvs);
  
  // Description:
  // Renders using Still/FullRes or interactive/LODs
  virtual void StillRender();
  virtual void InteractiveRender();

  // Description:
  // Are we currently in interactive mode?
  int IsInteractive() { return this->Interactive; }
  
  // Description:
  // My version.
  vtkRenderer *GetRenderer();
  vtkRenderWindow *GetRenderWindow();
  const char *GetRenderWindowTclName() {return this->RenderWindowTclName;}

  // Description:
  // Change the background color.
  void SetBackgroundColor(float r, float g, float b);
  virtual void SetBackgroundColor(float *c) {this->SetBackgroundColor(c[0],c[1],c[2]);}

  // Description:
  // Get the tcl name of the renderer.
  vtkGetStringMacro(RendererTclName);
    
  // Description:
  // The render view keeps track of these times but does not use them.
  vtkGetMacro(StillRenderTime, double);
  vtkGetMacro(InteractiveRenderTime, double);
  
  // Description:
  // Callback for the triangle strips check button
  void SetUseTriangleStrips(int val);
  
  // Description:
  // Callback for the immediate mode rendering check button
  void SetUseImmediateMode(int val);
    
  // Description:
  // Change between parallel or perspective camera.
  // Since this is a camera manipulation, it does not have to be here.
  void SetUseParallelProjection(int val);

  // Description:
  // Used to temporarily disable rendering. Useful for collecting a few
  // renders and flusing them out at the end with one render
  vtkSetMacro(DisableRenderingFlag, int);
  vtkGetMacro(DisableRenderingFlag, int);
  vtkBooleanMacro(DisableRenderingFlag, int);

  // Description:
  // Get the size of the render window.
  int* GetRenderWindowSize();

  // Description:
  // Controls whether the render module invokes abort check events.
  vtkSetMacro(RenderInterruptsEnabled,int);
  vtkGetMacro(RenderInterruptsEnabled,int);
  vtkBooleanMacro(RenderInterruptsEnabled,int);

protected:
  vtkPVRenderModule();
  ~vtkPVRenderModule();

  // This collection keeps a reference to all PartDisplays created
  // by this module.
  vtkCollection* PartDisplays;

  // This is used before a render to make sure all visible sources
  // have been updated.  It returns 1 if all the data has been collected
  // and the render should be local.
  virtual int UpdateAllPVData(int interactive);
 
  vtkPVApplication* PVApplication;
  vtkRenderer*      Renderer;
  vtkRenderWindow*  RenderWindow;

  int Interactive;
  
  char *RendererTclName;
  vtkSetStringMacro(RendererTclName);  
   
  char *RenderWindowTclName;
  vtkSetStringMacro(RenderWindowTclName);  
    
  double StillRenderTime;
  double InteractiveRenderTime;

  int DisableRenderingFlag;
  int RenderInterruptsEnabled;

  unsigned long ResetCameraClippingRangeTag;

  vtkPVRenderModule(const vtkPVRenderModule&); // Not implemented
  void operator=(const vtkPVRenderModule&); // Not implemented
};


#endif
