/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRenderView.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

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
#include "vtkRenderWindowInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkTreeComposite.h"

class vtkPVApplication;
class vtkKWInteractor;

class VTK_EXPORT vtkPVRenderView : public vtkKWView
{
public:
  static vtkPVRenderView* New();
  vtkTypeMacro(vtkPVRenderView,vtkKWView);

  // Description:
  // Set the application right after construction.
  void CreateRenderObjects(vtkPVApplication *pvApp);
  
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
  // Reset the camera clipping range based on the bounds of the
  // visible actors. This ensures that no props are cut off
  void ResetCameraClippingRange();

  // Description:
  // This method is executed in all processes.
  void AddComposite(vtkKWComposite *c);
  void RemoveComposite(vtkKWComposite *c);

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

  void OffScreenRenderingOn();

  // Description:
  // This is for an experiment on rendering timing.  It should be temporary.
  vtkTreeComposite *GetComposite() {return this->Composite;}

  // Description:
  // Update the navigation window for a particular source
  void UpdateNavigationWindow(vtkPVSource *currentSource);

  // Description:
  // Get the frame for the navigation window
  vtkGetObjectMacro(NavigationFrame, vtkKWLabeledFrame);
  
  void AButtonPress(int num, int x, int y);
  void AButtonRelease(int num, int x, int y);
  void Button1Motion(int x, int y);
  void Button2Motion(int x, int y);
  void Button3Motion(int x, int y);

  // Description:
  // Bound events are forwarded to this interactor.
  void SetInteractor(vtkKWInteractor *interactor);
  vtkKWInteractor *GetInteractor() 
    {return this->CurrentInteractor;}

  // Description:
  // Save the renderer and render window to a file.
  void Save(ofstream *file);
  void AddActorsToFile(ofstream *file);

  // Description:
  // Change the background color.
  void SetBackgroundColor(float r, float g, float b);

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
  
protected:

  vtkPVRenderView();
  ~vtkPVRenderView();
  vtkPVRenderView(const vtkPVRenderView&) {};
  void operator=(const vtkPVRenderView&) {};

  int Interactive;

  vtkTreeComposite *Composite;
  char *CompositeTclName;
  vtkSetStringMacro(CompositeTclName);

  char *RendererTclName;
  vtkSetStringMacro(RendererTclName);  
   
  char *RenderWindowTclName;
  vtkSetStringMacro(RenderWindowTclName);  
  
  vtkKWLabeledFrame *NavigationFrame;
  vtkKWWidget       *NavigationCanvas;

  vtkKWInteractor *CurrentInteractor;

  int EventuallyRenderFlag;
};


#endif





