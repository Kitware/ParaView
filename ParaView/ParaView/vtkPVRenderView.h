/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderView.h
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
// .NAME vtkPVRenderView - For using styles
// .SECTION Description
// This is a render view that is a parallel object.  It needs to be cloned
// in all the processes to work correctly.  After cloning, the parallel
// nature of the object is transparent.
// Other features:
// I am going to try to divert the events to a vtkInteractorStyle object.
// I also have put compositing into this object.  I had to create a separate
// renderwindow and renderer for the off screen compositing (Hacks).
// Eventually I need to merge these back into the main renderer and renderer
// window.


#ifndef __vtkPVRenderView_h
#define __vtkPVRenderView_h

#include "vtkKWView.h"

class vtkKWLabel;
class vtkKWScale;
class vtkKWPushButton;
class vtkLabeledFrame;
class vtkPVApplication;
class vtkPVData;
class vtkPVNavigationWindow;
class vtkPVSource;
class vtkPVTreeComposite;
class vtkPVWindow;

class VTK_EXPORT vtkPVRenderView : public vtkKWView
{
public:
  static vtkPVRenderView* New();
  vtkTypeMacro(vtkPVRenderView,vtkKWView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the application right after construction.
  void CreateRenderObjects(vtkPVApplication *pvApp);
  
  // Description:
  // Add widgets to vtkKWView's notebook
  virtual void CreateViewProperties();
  
  // Description:
  // Pack the properties so that they exapnd with the split frame.
  virtual void PackProperties();

  // Description:
  // Create the TK widgets associated with the view.
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Compute the bounding box of all the visibile props
  // Used in ResetCamera() and ResetCameraClippingRange()
  void ComputeVisiblePropBounds( float bounds[6] ); 
  
  // Description:
  // Method called by the toolbar reset camera button.
  void ResetCamera();
  
  // Description:
  // Specify the position, focal point, and view up of the camera.
  void SetCameraState(float p0, float p1, float p2,
                      float fp0, float fp1, float fp2,
                      float up0, float up1, float up2);

  // Description:
  // Reset the camera clipping range based on the bounds of the
  // visible actors. This ensures that no props are cut off
  void ResetCameraClippingRange();

  // Description:
  // Callback to set the view up or down one of the three axes.
  void StandardViewCallback(float x, float y, float z);

  // Description:
  // This method is executed in all processes.
  void AddPVData(vtkPVData *pvc);
  void RemovePVData(vtkPVData *pvc);

  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  // Description:
  // Computes the reduction factor to use in compositing.
  void StartRender();
  
  // Description:
  // Composites
  void Render();
  void EventuallyRender();
  void EventuallyRenderCallBack();
  
  // Description:
  // Update all the actors.
  void Update();

  // Description:
  // Callback method bound to expose events.
  void Exposed();
  
  // Description:
  // Are we currently in interactive mode?
  int IsInteractive() { return this->Interactive; }
  
  // Description:
  // My version.
  vtkRenderer *GetRenderer();
  vtkRenderWindow *GetRenderWindow();

  // Description:
  // This is for an experiment on rendering timing.  It should be temporary.
  vtkPVTreeComposite *GetComposite() {return this->Composite;}

  // Description:
  // Update the navigation window for a particular source
  void UpdateNavigationWindow(vtkPVSource *currentSource);

  // Description:
  // Get the frame for the navigation window
  vtkGetObjectMacro(NavigationFrame, vtkKWLabeledFrame);
  
  void AddBindings();
  
  // Description:
  // Save the renderer and render window to a file.
  void SaveInTclScript(ofstream *file, int vtkFlag, int offScreenFlag);
  void AddActorsToTclScript(ofstream *file);

  // Description:
  // Change the background color.
  void SetBackgroundColor(float r, float g, float b);
  virtual void SetBackgroundColor(float *c) {this->SetBackgroundColor(c[0],c[1],c[2]);}

  // Description:
  // Close the view - called from the vtkkwwindow. This default method
  // will simply call Close() for all the composites. Can be overridden.
  virtual void Close();

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
  // Callback for toggling between parallel and perspective.
  void ParallelProjectionCallback();
  
  // Description:
  // Callback for the triangle strips check button
  void TriangleStripsCallback();
  
  // Description:
  // Callback for the immediate mode rendering check button
  void ImmediateModeCallback();
  
  // Description:
  // Callback for the interrupt render check button
  void InterruptRenderCallback();
  
  // Description:
  // Callback for the use char check button.  
  // These are only public because they are callbacks.
  // Cannot be used from a script because they do not 
  // change the state of the check.
  void CompositeWithFloatCallback();
  void CompositeWithRGBACallback();
  void CompositeCompressionCallback();
  
  // Description:
  // Get the triangle strips check button.
  vtkGetObjectMacro(TriangleStripsCheck, vtkKWCheckButton);
  
  // Description:
  // Get the immediate mode rendering check button.
  vtkGetObjectMacro(ImmediateModeCheck, vtkKWCheckButton);

  // Description:
  // A convience method to get the PVWindow.
  vtkPVWindow *GetPVWindow();

  // Description:
  // Callback for reduction check
  void ReductionCheckCallback();
  
  // Description:
  // Callback for frame rate slider
  void FrameRateScaleCallback();

  // Description:
  // Used to temporarily disable rendering. Useful for collecting a few
  // renders and flusing them out at the end with one render
  vtkSetMacro(DisableRenderingFlag, int);
  vtkGetMacro(DisableRenderingFlag, int);
  vtkBooleanMacro(DisableRenderingFlag, int);
    
protected:

  vtkPVRenderView();
  ~vtkPVRenderView();

  void CalculateBBox(char* name, int bbox[4]);

  int Interactive;

  int UseReductionFactor;
  
  vtkPVTreeComposite *Composite;
  char *CompositeTclName;
  vtkSetStringMacro(CompositeTclName);

  char *RendererTclName;
  vtkSetStringMacro(RendererTclName);  
   
  char *RenderWindowTclName;
  vtkSetStringMacro(RenderWindowTclName);  
  
  vtkKWLabeledFrame *NavigationFrame;
  vtkKWWidget       *NavigationCanvas;
  vtkKWWidget       *NavScrollBar;

  vtkKWLabeledFrame *StandardViewsFrame;
  vtkKWPushButton   *XMaxViewButton; 
  vtkKWPushButton   *XMinViewButton; 
  vtkKWPushButton   *YMaxViewButton; 
  vtkKWPushButton   *YMinViewButton; 
  vtkKWPushButton   *ZMaxViewButton; 
  vtkKWPushButton   *ZMinViewButton; 

  vtkKWLabeledFrame *RenderParametersFrame;
  vtkKWCheckButton *TriangleStripsCheck;
  vtkKWCheckButton *ParallelProjectionCheck;
  vtkKWCheckButton *ImmediateModeCheck;
  vtkKWCheckButton *InterruptRenderCheck;
  vtkKWCheckButton *CompositeWithFloatCheck;
  vtkKWCheckButton *CompositeWithRGBACheck;
  vtkKWCheckButton *CompositeCompressionCheck;

  vtkKWCheckButton *ReductionCheck;  
  vtkKWWidget *FrameRateFrame;
  vtkKWLabel *FrameRateLabel;
  vtkKWScale *FrameRateScale;

  vtkPVNavigationWindow *NavigationWindow;
  
  int EventuallyRenderFlag;
  char* RenderPending;
  vtkSetStringMacro(RenderPending);

  double StillRenderTime;
  double InteractiveRenderTime;
  double StillCompositeTime;
  double InteractiveCompositeTime;

  // For the renderer in a separate toplevel window.
  vtkKWWidget *TopLevelRenderWindow;

  int DisableRenderingFlag;

  vtkPVRenderView(const vtkPVRenderView&); // Not implemented
  void operator=(const vtkPVRenderView&); // Not implemented
};


#endif
