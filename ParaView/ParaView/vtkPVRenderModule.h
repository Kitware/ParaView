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
// .NAME vtkPVRenderModule - Mangages rendering and LODs.
// .SECTION Description

// I am in the process of moving features around into new objects.

// This object manages creation and manipulation of the render window.  
// No user interface in this class.
// Some common camera manipulation (and other render window control)
// may be moved back into vtkPVRenderView to simplify this module.

#ifndef __vtkPVRenderModule_h
#define __vtkPVRenderModule_h

#include "vtkObject.h"

class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVData;
class vtkPVSource;
class vtkPVSourceList;
class vtkPVTreeComposite;
class vtkPVWindow;
class vtkPVRenderView;
class vtkRenderer;
class vtkRenderWindow;

class VTK_EXPORT vtkPVRenderModule : public vtkObject
{
public:
  static vtkPVRenderModule* New();
  vtkTypeRevisionMacro(vtkPVRenderModule,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // A conveniance method that I do not mean to be permenant !!!!!
  vtkPVWindow* GetPVWindow();

  // Description:
  // Set the application right after construction.
  void SetPVApplication(vtkPVApplication *pvApp);
  vtkGetObjectMacro(PVApplication, vtkPVApplication);

  // Description:
  // Need the render view for compositeManager
  void SetPVRenderView(vtkPVRenderView* pvView);

  // Description:
  // Compute the bounding box of all the visibile props
  // Used in ResetCamera() and ResetCameraClippingRange()
  void ComputeVisiblePropBounds( float bounds[6] ); 
  

  // Description:
  // This method is executed in all processes.
  void AddPVSource(vtkPVSource *pvs);
  void RemovePVSource(vtkPVSource *pvs);
  
  // Description:
  // Computes the reduction factor to use in compositing.
  void StartRender();
  
  // Description:
  // Composites
  void StillRender();
  void InteractiveRender();

  // Description:
  // Are we currently in interactive mode?
  int IsInteractive() { return this->Interactive; }
  
  // Description:
  // My version.
  vtkRenderer *GetRenderer();
  vtkRenderWindow *GetRenderWindow();
  const char *GetRenderWindowTclName() {return this->RenderWindowTclName;}

  // Description:
  // The center of rotation picker needs the compositers zbuffer.
  // Remove this method.  Change picking the center of rotation.
  vtkPVTreeComposite *GetComposite() {return this->Composite;}
  vtkGetStringMacro(CompositeTclName);

  // Description:
  // Change the background color.
  void SetBackgroundColor(float r, float g, float b);
  virtual void SetBackgroundColor(float *c) {this->SetBackgroundColor(c[0],c[1],c[2]);}

  // Description:
  // This method Sets all IVars to NULL and unregisters
  // vtk objects.  This should eliminate circular references.
  void PrepareForDelete();
  
  // Description:
  // Get the tcl name of the renderer.
  vtkGetStringMacro(RendererTclName);
  
  // Description:
  // Set this flag to indicate whether to calculate the reduction factor for
  // use in tree composite.
  vtkSetMacro(UseReductionFactor, int);
  vtkGetMacro(UseReductionFactor, int);
  vtkBooleanMacro(UseReductionFactor, int);
  
  // Description:
  // The render view keeps track of these times but does not use them.
  vtkGetMacro(StillRenderTime, double);
  vtkGetMacro(InteractiveRenderTime, double);
  vtkGetMacro(StillCompositeTime, double);
  vtkGetMacro(InteractiveCompositeTime, double);
  
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
  void SetUseCompositeWithFloat(int val);
  void SetUseCompositeWithRGBA(int val);
  void SetUseCompositeCompression(int val);
  
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

protected:
  vtkPVRenderModule();
  ~vtkPVRenderModule();

  // This is used before a render to make sure all visible sources
  // have been updated.  It returns 1 if all the data has been collected
  // and the render should be local.
  int UpdateAllPVData(int interactive);
 
  vtkPVApplication* PVApplication;
  vtkRenderer*      Renderer;
  vtkRenderWindow*  RenderWindow;

  int Interactive;

  int UseReductionFactor;
  
  vtkPVTreeComposite *Composite;
  char *CompositeTclName;
  vtkSetStringMacro(CompositeTclName);

  char *RendererTclName;
  vtkSetStringMacro(RendererTclName);  
   
  char *RenderWindowTclName;
  vtkSetStringMacro(RenderWindowTclName);  
    
  double StillRenderTime;
  double InteractiveRenderTime;
  double StillCompositeTime;
  double InteractiveCompositeTime;

  int DisableRenderingFlag;

  float LODThreshold;
  int LODResolution;
  float CollectThreshold;

  vtkPVRenderModule(const vtkPVRenderModule&); // Not implemented
  void operator=(const vtkPVRenderModule&); // Not implemented
};


#endif
